# Source after setting project_root. No generated data is written before validation.
export PYTHONDONTWRITEBYTECODE=1
workspace_environment="$(python3 -B "$project_root/scripts/lib/build_paths.py" env --root "$project_root")" || return
eval "$workspace_environment"
