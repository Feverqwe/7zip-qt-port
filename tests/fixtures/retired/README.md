`cramfs.img.zlib` contains only `data.bin` with the generated text
`retired CramFS fixture\n`. It was produced by this repository's former
deterministic CramFS fixture builder and read successfully with the original
26.05 runtime before removal. It is retained to check that CramFS cannot open
through either automatic detection or an explicit format request.
