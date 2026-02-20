# Command‑line tools

`cwipc` provides a set of standalone command‑line utilities that can be used
without writing any code.  Until mid‑2024 there were several binaries; the
current entry point is the single `cwipc` program with subcommands (similar to
`git`).

Run `cwipc --help` to get a list of available commands.  Typical tools include:

* `cwipc check` – verify the installation and third‑party dependencies.
* `cwipc register` – assist with camera calibration and registration.
* `cwipc grab` – capture point clouds, convert between formats, compress/decompress.
* `cwipc view` – render a live or recorded point cloud stream.
* `cwipc forward` – stream point clouds over the network.

Each subcommand also supports `--help` for detailed usage information.  The
utilities are identical on all supported platforms and share a common set of
logging and configuration options.

### Streaming example

Start a server on port 4303:

```sh
cwipc forward --port 4303
```

Connect with a viewer on the same or another machine:

```sh
cwipc view --netclient localhost:4303
```

Additional examples (recording, playback, synthetic streams) appear in the
README and in `doc/raw-recording.md`.
