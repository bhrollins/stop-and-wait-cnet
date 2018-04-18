#include <cnet.h>
#include <string.h>

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
uint32_t expected;        // expected acknowledgment number
Segment ack_buffer;
CnetTimerID reack_buffer_timer;

// Variables for the transmitter
uint32_t transmitted;     // sequence number
Segment send_buffer;
CnetTimerID retransmit_timer;

// ------------------------- send frame ---------------------------------------
EVENT_HANDLER(send_frame) {

  int destination;
  SegmentHeader header = {};
  uint8_t buffer[MAX_MESSAGE_SIZE];
  size_t length;

  header.length = MAX_MESSAGE_SIZE;

  // get frame from application layer
  CHECK(CNET_read_application(&destination, &buffer, &(header.length)));
  // disable application until ack is received:
  CNET_disable_application(ALLNODES);

  // start retransmit timer
  retransmit_timer = CNET_start_timer(EV_TIMER0, 5000000, 0);

  header.type = DataSegment;
  header.sequence_number = transmitted++;
  header.crc = 0;

  send_buffer.header = header;
  memcpy(&send_buffer.data, (char *)buffer, header.length);

  length = send_buffer.header.length + sizeof(SegmentHeader);
  send_buffer.header.crc = CNET_crc32((unsigned char *)&send_buffer, length);

  // send frame down the wire
  printf("Sending data segment of size: %d. Seq number: %d.\n", length, transmitted);
  CHECK(CNET_write_physical(1, &send_buffer, &length));

}

// ------------------------ retransmit timer handler --------------------------
EVENT_HANDLER(retransmit_timer_handler) {
  size_t length;

  length = send_buffer.header.length + sizeof(SegmentHeader);
  printf("Resending data segment of size %d. Seq number: %d.\n", length, transmitted);
  CHECK(CNET_write_physical(1, &send_buffer, &length));
  retransmit_timer = CNET_start_timer(EV_TIMER0, 5000000, 0);
}

// ----------------------------- frame arrived --------------------------------
EVENT_HANDLER(frame_arrived) {
  Segment buffer;
  int link;
  size_t length;
  length = MAX_MESSAGE_SIZE + sizeof(SegmentHeader);

  CHECK(CNET_read_physical(&link, &buffer, &length));

  uint32_t crc, post_crc;
  crc = buffer.header.crc;
  buffer.header.crc = 0;
  post_crc = CNET_crc32((unsigned char *)&buffer, length);

  if (post_crc != crc) {
    printf("Checksums are not equal, allowing timeout.\n");
    return; // do nothing -- let retransmit timer go off
  }

  switch (buffer.header.type) {
    case DataSegment:
      printf("Received data segment of size: %d.\n", length);
      if (buffer.header.sequence_number == expected){ // otherwise let timeout occur
        // stop the reack timer
        if ( NULLTIMER != reack_buffer_timer)
          CHECK(CNET_stop_timer( reack_buffer_timer ));

        // send message to the application layer
        length = buffer.header.length;
        CHECK(CNET_write_application(&buffer.data, &length));
        expected++;

        // send acknowledgment (store in ack_buffer until next expected data frame)
        SegmentHeader header = {};
        header.type = AcknowledgementSegment;
        header.sequence_number = expected;
        header.length = 0;
        header.crc = 0;

        ack_buffer.header = header;
        uint8_t nil[MAX_MESSAGE_SIZE] = {0};
        memcpy(&ack_buffer.data, nil, header.length);

        length = ack_buffer.header.length + sizeof(SegmentHeader);
        ack_buffer.header.crc = CNET_crc32((unsigned char *)&ack_buffer, length);
        printf("Sending acknowledgment of size: %d. Exp number: %d.\n", length, expected);
        CHECK(CNET_write_physical(1, &ack_buffer, &length));

        // start reacknowledgement timer
        reack_buffer_timer = CNET_start_timer(EV_TIMER1, 5000000, 0);
      }
      break;

    case AcknowledgementSegment:
      printf("Received acknowledgment segment of size: %d.\n", length);
      if (buffer.header.sequence_number == transmitted){
        if ( NULLTIMER != retransmit_timer )
          CHECK(CNET_stop_timer( retransmit_timer ));
        if (nodeinfo.nodenumber == 0)
          CHECK(CNET_enable_application(ALLNODES));
      }
      break;

    default:
      printf("Received an unsupported frame type.\n");
  }

}

// ------------------- reacknowledgement timer handler ------------------------
EVENT_HANDLER(reacknowledge_timer_handler) {
  size_t length;
  length = ack_buffer.header.length + sizeof(SegmentHeader);

  printf("Resending acknowledgment of size: %d. Exp number: %d.\n", length, expected);
  // resend acknowledgment
  CHECK(CNET_write_physical(1, &ack_buffer, &length));
  reack_buffer_timer = CNET_start_timer(EV_TIMER1, 5000000, 0);
}

// ------------------------------- init op ------------------------------------
EVENT_HANDLER(reboot_node) {
  transmitted = 0;
  expected = 0;

  CHECK(CNET_set_handler(EV_PHYSICALREADY, frame_arrived, 0));
  CHECK(CNET_set_handler(EV_APPLICATIONREADY, send_frame, 0));
  CHECK(CNET_set_handler(EV_TIMER0, retransmit_timer_handler, 0));
  CHECK(CNET_set_handler(EV_TIMER1, reacknowledge_timer_handler, 0));

  if (nodeinfo.nodenumber == 0)
    CHECK(CNET_enable_application(ALLNODES));

}
