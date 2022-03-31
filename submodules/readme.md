# Submodules

The following submodules are used by this project:

- `boringssl`: BoringSSL cryptography library used by LSQUIC
- `lsquic`: QUIC implementation used by project
- `gst-build`: Builds GStreamer modules. It is necessary to build from source as modifications to GStreamer elements were required.
- `mininet`: Mininet network simulation framework used during evaluation.
- `wireshark`: Builds TShark packet analyser. Newer version required so that TShark can decrypt QUIC.

The following modifications were made to LSQUIC:

- Restrict each packet to a single stream. This avoids the scenario where a single loss affects multiple streams.
- Send packets immediately. Buffering removed so that stream FRAMES are sent immediately, avoiding sending delay that waiting for enough data to fill a packet would cause.
- Fix bug where new streams would cause data to be sent even when there was no room in the congestion window.
- Allow max streams to be set via engine settings.
- Reduce maximum ACK delay to 5 ms, to reduce delay before a loss is detected.
  
Additionally, the following GStreamer element modifications were made:

- Add option to identity element to sync incoming buffers. This allows us to simulate a capture rate of 24fps.
- Adapt JitterBuffer for HOL blocking network protocols. The jitterbuffer only detects loss when subsequent packets arrive. On Streams with HOL blocking, new packets will not arrive, so the jitterbuffer will not work for HOL blocked streams without modification. 