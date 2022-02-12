# Logging notes for gstreamer

In the server side logs, rtph264pay will note the type of each Nal unit. From this type we can derive whether this data contained is necessary for decoding (see Keyframe_notes.md).

After the Nal type is logged, the last rtp seq num for the group of rtp packets which will carry this unit will subsequently be logged.


On the client side, if all the packets carrying a certain Nal unit arrive, the Nal type will be logged as part of a "handle NAL type" message. The seq num of every rtp packet that is successfully delivered to the rtph264depay element will also be logged. If any of the rtp packets which carry fragments of a NAL unit are considered lost, we will not see the "handle NAl type" message.

Thus if we take from the server side:

- The time of each Nal unit arrives at the rtppayloader
- The type of each Nal unit
- The last rtp seq num for each unit

and on the client side we take::

- The depature of each complete Nal unit from the rtpdepayloader (passed on to decoder)
- The type of each complete Nal unit
- The last rtp seq num seen before completeing of each Nal unit

Then we will be able to determine the app latency for each nal unit. For each Nal unit that we do not see "handle NAl type" on the client side, we can mark as incomplete (-1 latency). This will also allow us to calcuate the number of useful frames.