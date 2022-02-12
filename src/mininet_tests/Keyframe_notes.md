# Key Frame notes

We are working with h264 encoded video taken from a zoom recording.

### Frame types:

Our test video, contains only P-frames and I-frames. As a consequence, no frame requires a reference to future frames in order to be decoded. 

Thus the following frames can be considered useful for playback:

- Complete I-frames
- All P-frames which follow a complete I-frame

and the following can be considered non-useful:

- Incomplete I-frames
- All P-frames following an incomplete or missing I-frame


### Nal units:

h264 video is seperated into Nal units. Our test video contains Nal units of type 1, 5, 7 and 8.

Nal type 1 units essentially hold frames which are not I frames (non-IDR). Thus we can identify P-frames as all units with Nal type 1. 

Nal type 5 units hold I-frames, and Nal type 7 and 8 hold metadata needed to decode those I-frames (resolution, framerate). 

Thus, for an I-frame to be considered complete, The Nal units of type 1 which make up the frame and the associated Nal units of type 7 and 8 must all be delivered intact.

