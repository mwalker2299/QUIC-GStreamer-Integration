# Test Framework Parameter Notes

This project aims to determine how fully and partially reliable QUIC implementations compare to UDP when used for low latency applications (such as VOIP and live broadcasts). Both latency and the quality of playback of each QUIC implementation and UDP will be compared. The final results will show whether or not QUIC can perform more reliably than UDP without impacting the latency of the stream.

To accomplish this, I will evaluate UDP and each of our QUIC implementations under different network conditions which emulate real world scenarios. This file describes the parameters that will vary between tests.

**NOTE: These were the original parameters that were going to be tested. However, the combination of all of the parameters below would take to0 long (over 30 days). These parameters were reduced to those found in parameters.json to allow testing to complete in a reasonable time. Initially, Jitter was still going to be included, but the first few test runs were bugged and so this parameter was cut to save time for the final test run.**


### Bandwidth

The maximum rate of data throughput across the network will vary between tests. This can be used to confirm the model behaves sensibly (if bandwidth is less than video bitrate, streaming should impossible). 

I would not expect bandwidths higher than the video bitrate to have much impact on tests results, but there is no harm in gathering this data and the analysis can be placed in the appendix if it is uninteresting.

Mininet allows he bandwidth to be set on links when they are created using the `bw` parameter.


A 2021 ofcom report[1] found that the average download speeds in UK homes could be split into the following ranges:

- 10 Mbit/s or less (8\%)
- 10 - 30 Mbit/s (16\%)
- 30 - 100 Mbit/s (54\%)
- 100 - 300 Mbit/s (18\%)
- 300 Mbit/s or greater (4\%)

So, in order to best represent the circumstances of UK residents, the range boundaries from the above data will be used in the experiments. Additionally, 1 Mbit/s will also to be tested as a sanity to check.

**Experiment Values:**

- 1   Mbit/s
- 10  Mbit/s
- 30  Mbit/s 
- 100 Mbit/s 
- 300 Mbit/s 

### Buffer Delay

Another additional parameter is buffer delay on the client side. A larger buffering delay will result in increased latency but smaller buffers may not allow time to compensate for jitter or packet loss.

Gstreamer provides the element rtpjitterbuffer element. As the name implies, this element acts as a jitter buffer, temporarily storing incoming rtp packets to allow time for delayed packets to arrive. The element will also reorder any packets which arrive out of order due to network jitter. 

rtpjitterbuffer has two features which are relevant to our experiment. The first is a latency property, which allows us to configure the maximum latency/buffer delay of the rtpjitterbuffer. The second is the ability to extract statistics which will be useful for analysis such as:

- `num-pushed`: the number of packets pushed out.
- `num-lost`: the number of packets considered lost.
- `num-late`: the number of packets arriving too late.
- `num-duplicates`: the number of duplicate packets.
- `avg-jitter`: the average jitter in nanoseconds.

It has been difficult to find recommend values for a fixed jitter buffer delay. It appears that most low latency applications use a dynamic jitter buffer but the gstreamer element does not support this. However, the 2018 ofcom home broadband report[2] found that VOIP applications which use a fixed buffer have an average buffer delay of 20ms.

The default buffer delay for the rtpjitterbuffer element is much higher, at 200ms. A delay this great would be too large for an interactive or VOIP application but would be acceptable for live streaming.

Additionally, as mentioned below, the 2018 ofcom report also found that the peak and average downstream jitter values were between 0ms and 1ms for ADSL2+, FTTC, and Cable for all providers. As a result, I believe that experimenting with a jitter buffer delay of 1ms may provide interesting results as well.

Finally, for completeness, it would be interesting to see how each implementation performed without a jitter buffer.

**Experiment Values:**

- No jitter buffer
- 1ms jitter buffer
- 20ms  jitter buffer
- 200ms jitter buffer


### Router Queue Length

Upon arrival at a router or switch, packets must be processed before they are transmitted to the appropriate node in the network. If packets arrive at a greater rate than they can be processed, then they are placed into a queue.

RFC 8868 provides recommended values for router queue length:

- QoS-aware (or short): 70 ms
- Nominal: 300-500 ms
- Buffer-bloated: 1000-2000 ms

These are expressed in milliseconds where:

QueueSize (in bytes) = QueueSize (in sec) x Throughput (in bps)/8

From what I have read, throughput in this case would be equivalent to our bandwidth. So, for each test at a given bandwidth, we will calculate the needed queue length in bytes such that our queue lengths are:

**Experiment Values:**

- 70ms
- 300ms
- 1000ms

However, Colin has suggested that, rather than use fixed values, we should use the following formula:

max_throughput x delay x 1.5 (50% more for safety). Since we need to express the max queue size in packets, then this would be equal to:

- Bandwidth (in bit/s) x (MTU x 8 (for max packet size in bits)) x delay x 1.5

By default, the queue management algorithm used by mininet is tail-drop (new packets are dropped when the queue if full). This makes sense as, according to RFC 8868, tail-drop queue management is the prevalent in real world networks.


### Jitter

Packet Delay Variation (Jitter) is the variation in the propagation delay of individual packets with respect to the expected end-to-end propagation delay. Jitter can result in the reordering of packets or unexpected delays in the arrival of packets.

Mininet links can be configured to introduce jitter to the network.

When researching reasonable jitter values I've noticed a discrepancy in the recommended experimental value in RFC 8867 and the real world values measured by ofcom. RFC 8867 suggests an end to end of 30ms in experiments. However, the downstream jitter values reported by ofcom[2] are much smaller (0 - 1 ms). 

I was unable to find justification for the 30ms value recommended in RFC 8867. However, it easy to find many other articles which express 30ms as the upper limit for acceptable jitter on a network. 

**Experiment Values:**

- 0ms
- 1ms
- 30ms



### Packet Delay:

The end-to-end propagation delay of packets can vary greatly across real world networks and has a significant impact in the overall latency of a stream.

RFC 8868 suggests the following values for end-to-end packet delay:

- Very low latency: 0 ms
- Low latency: 50 ms
- High latency: 150 ms
- Extreme latency: 300 ms

The latency values reported by ofcom[2] fall between the "very low latency" and "low latency" values. ofcom reports a range of average latencies between 10ms and 30ms. In order to relate or experiment to real world scenarios, we should add these range boundaries to our list of experiment values. 

While, the higher latency values may not be relevant to the average UK household, the results of experiments using these values could still be valuable. They could represent edge cases such as a VOIP with a colleague in another part of the world (e.g. The west coast of the USA or Australia).

**Experiment Values:**

- 0 ms
- 10 ms
- 30 ms
- 50 ms
- 150 ms
- 300 ms

### Packet Loss:

The rate of packet loss can also vary greatly across real world networks and can impact both QoS (if lost packets are not retransmitted) and latency (if an application needs to wait for lost packets to be retransmitted.) 

In our case, we will be retransmitting packets, but any that do not arrive in time will be discarded by the de-jitter buffer. So varying our packet loss will give us insight into it's impact on the QOS provided by the different implementations under test.

RFC 8868 suggests the following values for packet loss:

- no loss: 0\%
- 1\%
- 5\%
- 10\%
- 20\%

The ofcom report values for average loss fall between 0.1% and 0.4%. As with packet delay, It would be good to perform an experiment using these range values so that we can relate the results to real world scenarios. Mininet's python API only accepts integer loss values by default, but the underlying tc command does accept floating point values, so it should not be difficult to modify mininet to accept floating point values.


**Experiment Values:**

- 0\%
- 0.1\%
- 0.4\%
- 1\%
- 5\%
- 10\%
- 20\%


## Cross-traffic 

To better model a real world network, cross traffic can be added. 

An additional server and client will be added to the dumbbell topology. For the duration of each experiment run, the client will download a file from the server.


## References 

\[1]: Ofcom. Home Broadband Performance – Technical Report. September 2021. URL: https://www.ofcom.org.uk/research-and-data/telecoms-research/broadband-research/broadband-speeds/uk-home-broadband-performance-march-2021
\[2]: Ofcom. The performance of fixed-line broadband delivered to UK residential consumers. November 2018. URL: https://www.ofcom.org.uk/__data/assets/pdf_file/0020/147332/home-broadband-report-2018.pdf