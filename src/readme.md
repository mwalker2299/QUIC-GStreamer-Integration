# Readme

The `src` directory contains two sub-directories `gst-quic` for the GStreamer plugins and `mininet_tests` for the test framework. This README first describes the contents of each directory, then provides build requirements, build steps, and steps for running the test framework

**GStreamer Plugins**

The `gst-quic` directory contains the GStreamer element plugins. For each method of sending there are a pair of plugins. One for the server-side 'sink' element which receives GstBuffers from upstream elements in the pipeline and sends it the connected client, and one for the client-side 'src' element which reads data from the network and passes it downstream in the form of GstBuffers.

There are 8 QUIC elements in total. Each element has a `.h` file which defines the element (parent class, properties, etc) and `.c` which contains the code needed to interface with the LSQUIC library, communicate with the rest of the pipeline, and handle buffers. The 8 elements can be grouped into 4 sink-src pairs:

- `gstquicsinkss` and `gstquicsrcss` communicate using a single QUIC stream. 
- `gstquicsinkgop` and `gstquicsrcgop` communicate using a stream for every group of pictures (GOP). Every new I-frame marks the start of a new stream and this I-frame and subsequent P-frames will all be written to that stream. 
- `gstquicsinkfps` and `gstquicsrcfps` communicate using a frame per stream. The PTS of GstBuffers changes with every new frame, and this indicates to the sink element that a new stream should be created.
- `gstquicsinkpps` and `gstquicsrcpps` use a new stream for every RTP packet sent. Each buffer received contains a single RTP packet and so a new stream is created for every buffer.

There is also a TCP element pair, `gsttcpsink` and `gsttcpsrc`, as the existing GStreamer TCP elements do not function correctly when transporting real-time multimedia.

For the TCP, single-stream, GOP-per-stream, and frame-per-stream 'src' elements expect that the length of each rtp packet is appended to the beginning as per RFC 4571. This allows them to check when a full packets worth of data has arrived on a stream. There is a gstreamer element which performs this functionality, but it was introducing significant delays during evaluation, so the functionality was placed into the elements directly. The packet-per-stream 'src' element does not need the length appended to the start of packets, as stream completion indicates that a full packet is available.

The implementation of the single-stream, GOP-per-stream, and frame-per-stream 'src' elements are very similar, as there is no need for the 'src' element to be aware of the strategy for splitting data into streams. I plan to unify these elements in the future.

The pipeline descriptions used in testing can be found in `implementations.json` in the `mininet_test` directory. The server-side pipelines will read a raw video file and encode the frames every 1/24 seconds to simulate a 24fps capture rate. The encoded frames will then be passed to an RTP payloader element, then if RFC 4571 is required, to a RTP stream payloader element. Finally, the RTP packets will be passed to the network element. On the client-side, the network elements will generate buffers and pass these to an RTP jitterbuffer which will hold packets until previous packets arrive or are declared lost. The RTP packets are then passed to an RTP depayloader element before being decoded.

**Test Framework**

The `mininet_tests` directory contains the python scripts used to evaluate the different QUIC and TCP elements described above. UDP was also evaluated, but the existing GStreamer elements functioned without issue (Actually 1 out of 100 runs failed to capture the first few packets sent when using the UDP elements. These tests had to be repeated.)

`mininet_tests` contains the following files:

- `network.py` defines a custom dumbbell toplogy using Mininet's Python API. The topology has two switches which are connected by a switch. The calling code can customise the properties of this link by passing link parameters. Additionally, while the default number of servers and clients is 1, any number of servers and clients can be created.
- `parameters.json` holds the parameters used by the tests. Propagation delay, loss ratio, network jitter, maximum jitter buffer delay and bandwidth can all be configured through this file. Additionally, `parameters.json` has an option for enabling cross traffic. However, this has not been fully implemented, and, if enabled, will simply spawn an additional client-server pair which ping each other throughout the test.
- `implementations.json` holds details about each network protocol implementation which will be tested. This includes the protocol name, the pipeline descriptions for the client-side and server-side pipelines, GStreamer logging levels, and the expected running time. **Note: The test framework provides up to 3 times the (running time + 5 seconds) for the pipelines to finish. This was initially necessary when CUBIC was being used as a congestion control algorithm, but after switching to BBR all tests completed within 75 seconds. The running time was been reduced to 20 seconds to match the 75 second maximum during final evaluation to save time.
- `test_loop.py` Runs a single test given parameters and a logging directory from the `test_runner.py` script. A Mininet net object is created using `network.py` and threads which run and monitor the server-side and client-side pipelines are created on the server and client node respectively. TShark threads are also created and attached to the switch nodes to monitor network traffic.
- `test_runner.py` is the test runner. It takes an output directory and number of iterations as cmdline parameters. It reads configuration information from the `parameters.json` and `implementations.json` files. For implementation and each possible combination of parameters, directories are created in the given output directory. Each test is then ran by passing the parameters and logging directory to `test_loop.py`. If `test_runner.py` detects that a given test has already run, it will skip it.
- `keep_tests_running.py` and `avoid_deadlock.py` where created to wrap around `test_runner.py`. This was necessary as mininet would often crash or stall. `keep_tests_running.py` restarts the tests whenever mininet crashes, and `avoid_deadlock.py` restarts the `keep_tests_running.py` every 30 minutes to lessen the impact of stalls.
- `generate_cert.sh` will create the SSL certificate and Key needed by QUIC. Used by `test_runner.py` at the start of a test run.
- `BBB_dec.raw` is the raw video data used by the tests. In order to run the tests, you will need to edit the `implementation.json` file, such that the location provided to the filesrc element points to `BBB_dec.raw` on your machine. 

**Note: The current test parameters will take about three days to run. Additionally, each test generates about 160MB of logs, so if the current test parameters are used, over 500GB of data will be generated. If you wish to observe a shorter, less storage intensive run, I would suggest reducing the parameter set**


### Build steps

The GStreamer element plugins and test framework were written and tested on a fresh Ubuntu 20.04 install. 

To install necessary libraries (e.g. my modified LSQUIC, my modifications to GStreamer, BoringSSL, TShark) run the install.sh script at the top level directory with sudo permissions:

This may take some time.

```
chmod u+x install.sh
sudo ./install.sh
```




### Test steps

In order to run the tests, we need to enter the GStreamer development environment so that we have access to my changes to the GStreamer.

Run the following command:

```
sudo ninja -C submodules/gst-build/build uninstalled
```

Now that we are in the development environment, we must navigate to the mininet_tests directory:

```
cd ../../src/mininet_tests
```

We also need to add our GStreamer elements to GStreamers search path:

```
export GST_PLUGIN_PATH=$GST_PLUGIN_PATH:/usr/lib/x86_64-linux-gnu/gstreamer-1.0
```

Now we can run the tests using `test_runner.py`:

```
python3 test_runner.py -o {$OUTPUT_DIR} -i {$ITERS}
```

If you are running many tests, you may wish to run the `avoid_deadlocks.sh` wrapper script instead:

```
./avoid_deadlocks.sh -o {$OUTPUT_DIR} -i {$ITERS}
```

If you see an error which claims a controller is already running on port 6653, then run thw following and try again:

```
sudo fuser -k 6653/tcp
```

**NOTE: As mentioned above, these tests generate a lot of logs (~160MB per test). I would strongly advise reducing the test parameter set inside `parameters.json` unless you have a terabyte external drive lying around.** 

