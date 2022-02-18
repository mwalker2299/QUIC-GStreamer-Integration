# Meeting Conducted at 14:00 on 26/01/21

* Evaluating the Effectiveness of QUIC When Integrated Into Gstreamer
* Matthew Walker
* 2310528
* Dr Colin Perkins


## Status Report

* Researched and chose parameters which will vary between tests as well as finding appropriate test values (These were sent to Colin prior to the meeting)
* Modified test framework to accept new parameters and introduce cross traffic
* Added timeout to test framework to handle stalls
* Began looking into analysis metrics.


## Minutes:

* Discussed parameter results and different networks which may experience high loss
* Colin is happy with the test params
* Colin suggested that an alternative to fixed queue size was to use bandwidth multiplied by delay
* Going up to 20\% might be unnecessary but would do no harm.
* Colin is happy with plan for analysis (see Analysis_Notes.md)
* We agreed I would research some video quality metrics and find one that fits best.


## Plan for next week

- Finish identifying evaluate metrics, provide these to Colin.
- Create analysis script to collect and analyse the selected metrics.





