#include <cnet.h>

typedef enum {DataSegment, AcknowledgementSegment} SegmentType;

typedef struct {
  SegmentType type;
  uint32_t sequence_number;
  size_t length;
  uint32_t crc;
} SegmentHeader;

typedef struct {
  SegmentHeader header;
  uint8_t data[MAX_MESSAGE_SIZE];
} Segment;

// Variables for the receiver
uint32_t expected;
Segment ack_buffer;
CnetTimerID reack_buffer_timer;

// Variables for the transmitter
Segment send_buffer;
uint32_t transmitted;
CnetTimerID retransmit_timer;

EVENT_HANDLER(send_frame) {
  // TODO Implement transmitting a frame
}

EVENT_HANDLER(retransmit_timer_handler) {
  // TODO Implement handling the retransmit timer going off
}

EVENT_HANDLER(frame_arrived) {
  // TODO Implement receiving a frame
}

EVENT_HANDLER(reacknowledge_timer_handler) {
  // TODO Implement the reacknowledgement timer going off
}

EVENT_HANDLER(reboot_node) {
  transmitted = 0;
  expected = 0;

  CHECK(CNET_set_handler(EV_PHYSICALREADY, frame_arrived, 0));
  CHECK(CNET_set_handler(EV_APPLICATIONREADY, send_frame, 0));
  CHECK(CNET_set_handler(EV_TIMER0, retransmit_timer_handler, 0));
  CHECK(CNET_set_handler(EV_TIMER1, reacknowledge_timer_handler, 0));

  if (nodeinfo.nodenumber == 0) {
    CHECK(CNET_enable_application(ALLNODES));
  }
}
