//! Preserve configured SSH arguments for cp2, whose SSH executable is fixed.
use std::{env, fs, io, process::{Command, ExitCode}};

fn run() -> io::Result<ExitCode> {
    let path = env::var_os("SEVENZIP_CP2_SSH_ARGV")
        .ok_or_else(|| io::Error::other("Missing cp2 SSH configuration"))?;
    let bytes = fs::read(path)?;
    let config = String::from_utf8(bytes).map_err(io::Error::other)?;
    let mut configured: Vec<&str> = config.split('\0').collect();
    let destination = configured.pop().ok_or_else(|| io::Error::other("Missing SSH destination"))?;
    let mut args: Vec<String> = env::args().skip(1).collect();
    if args.len() < 4 || args[0] != "-p" {
        return Err(io::Error::other("Unexpected cp2 SSH invocation"));
    }
    // cp2 supplies its default port/current local username. The configured
    // connection, including aliases, User and nondefault Port, is authoritative.
    args.drain(..3);
    let mut remote = args.pop().ok_or_else(|| io::Error::other("Missing remote command"))?;
    if remote == "cmd /c echo %PROCESSOR_ARCHITECTURE%" {
        if let Ok(probe) = env::var("SEVENZIP_CP2_WINDOWS_PROBE") {
            remote = probe;
        }
    }
    let executable = configured.first().ok_or_else(|| io::Error::other("Missing SSH executable"))?;
    let mut command = Command::new(executable);
    // NAS workspaces cannot host Unix sockets. Key authentication permits
    // independent sessions without a local multiplexing socket.
    command.args(["-o", "ControlMaster=no", "-o", "ControlPath=none"]);
    command.args(&configured[1..]).args(args).arg(destination).arg(remote);
    #[cfg(unix)]
    {
        use std::os::unix::process::CommandExt;
        Err(command.exec())
    }
    #[cfg(not(unix))]
    {
        let status = command.status()?;
        Ok(ExitCode::from(status.code().unwrap_or(1) as u8))
    }
}

fn main() -> ExitCode {
    match run() {
        Ok(code) => code,
        Err(error) => { eprintln!("cp2 SSH: {error}"); ExitCode::FAILURE }
    }
}
