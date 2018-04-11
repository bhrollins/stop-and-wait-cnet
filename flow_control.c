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

  // read application to send buffer
  // send buffer sequence number = transmitted
  // write physical
  // disable application
}

EVENT_HANDLER(retransmit_timer_handler) {
  // TODO Implement handling the retransmit timer going off
}

EVENT_HANDLER(frame_arrived) {
  // TODO Implement receiving a frame

  // generate CRC for new package
  // check new CRC with sent CRC
  // if CRC matches, check with expected if it is the expected frame
  // if CRC, and expected match, send ACKNOWLEGMENT

  // ???? length = buffer.header.length + sizeof( SegmentHeader );

  Segment buffer;

  CHECK(
    CNET_read_physical(
      1, &buffer, &buffer.length
    )
  );

  if (buffer.length == 0) {
    // TODO check buffer.sequence_number to transmitted
      // if true, transmitted ++
      // send new packet
      // reset resend_ack timer

    if (buffer.sequence_number == transmitted) {
      transmitted++;

      // Get new packet
      CHECK(
        CNET_read_application(
          &destination, &send_buffer, &(send_buffer.header.length)
        )
      );

      send_buffer.sequence_number == transmitted;

    }
  }

  // Generate a new CRC code on the sent pack
  // Check the generated CRC code against the CRC sent with package
  if (buffer.crc == CNET_crc32(&ack_buffer, buffer.header.length + sizeof( SegmentHeader ))) {

    // Check to see if package is expected
    if (buffer.sequence_number == expected) {
      expected++;

      ack_buffer.sequence_number = buffer.sequence_number;
      // set to 0 cause we're not sending data
      ack_buffer.header.length = 0;

      // TODO Write to application

      // TODO Send ACKNOWLEGMENT
      CHECK(
        CNET_write_physical(
          1, &ack_buffer, &length
        )
      );

      // TODO Reset reacknowledge_timer_handler
    }
    else {
      printf("ERROR: Sequence_number %d received. Expected %d.\n", ack_buffer.sequence_number, expected);
    }
  }
  else {
    printf("ERROR: CRC check failed.\n");
  }
}

EVENT_HANDLER(reacknowledge_timer_handler) {
  // TODO Implement the reacknowledgement timer going off

  // TODO Send ACKNOWLEGMENT

  // TODO Reset reacknowledge_timer_handler
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
