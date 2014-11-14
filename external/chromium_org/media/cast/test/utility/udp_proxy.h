// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_CAST_TEST_UTILITY_UDP_PROXY_H_
#define MEDIA_CAST_TEST_UTILITY_UDP_PROXY_H_

#include <vector>

#include "base/basictypes.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/single_thread_task_runner.h"
#include "media/cast/transport/cast_transport_config.h"
#include "net/base/ip_endpoint.h"

namespace net {
class NetLog;
};

namespace base {
class TickClock;
};

namespace media {
namespace cast {
namespace test {

class PacketPipe {
 public:
  PacketPipe();
  virtual ~PacketPipe();
  virtual void Send(scoped_ptr<transport::Packet> packet) = 0;
  // Allows injection of fake test runner for testing.
  virtual void InitOnIOThread(
      const scoped_refptr<base::SingleThreadTaskRunner>& task_runner,
      base::TickClock* clock);
  virtual void AppendToPipe(scoped_ptr<PacketPipe> pipe);
 protected:
  scoped_ptr<PacketPipe> pipe_;
  // Allows injection of fake task runner for testing.
  scoped_refptr<base::SingleThreadTaskRunner> task_runner_;
  base::TickClock* clock_;
};

// A UDPProxy will set up a UDP socket and bind to |local_port|.
// Packets send to that port will be forwarded to |destination|.
// Packets send from |destination| to |local_port| will be returned
// to whoever sent a packet to |local_port| last. (Not counting packets
// from |destination|.) The UDPProxy will run a separate thread to
// do the forwarding of packets, and will keep doing so until destroyed.
// You can insert delays and packet drops by supplying a PacketPipe.
// The PacketPipes may also be NULL if you just want to forward packets.
class UDPProxy {
 public:
  virtual ~UDPProxy() {}
  static scoped_ptr<UDPProxy> Create(const net::IPEndPoint& local_port,
                                     const net::IPEndPoint& destination,
                                     scoped_ptr<PacketPipe> to_dest_pipe,
                                     scoped_ptr<PacketPipe> from_dest_pipe,
                                     net::NetLog* net_log);
};

// The following functions create PacketPipes which can be linked
// together (with AppendToPipe) and passed into UdpProxy::Create below.

// This PacketPipe emulates a buffer of a given size. Limits our output
// from the buffer at a rate given by |bandwidth| (in megabits per second).
// Packets entering the buffer will be dropped if there is not enough
// room for them.
scoped_ptr<PacketPipe> NewBuffer(size_t buffer_size, double bandwidth);

// Randomly drops |drop_fraction|*100% of packets.
scoped_ptr<PacketPipe> NewRandomDrop(double drop_fraction);

// Delays each packet by |delay_seconds|.
scoped_ptr<PacketPipe> NewConstantDelay(double delay_seconds);

// Delays packets by a random amount between zero and |delay|.
// This PacketPipe can reorder packets.
scoped_ptr<PacketPipe> NewRandomUnsortedDelay(double delay);

// This PacketPipe inserts a random delay between each packet.
// This PacketPipe cannot re-order packets. The delay between each
// packet is asically |min_delay| + random( |random_delay| )
// However, every now and then a delay of |big_delay| will be
// inserted (roughly every |seconds_between_big_delay| seconds).
scoped_ptr<PacketPipe> NewRandomSortedDelay(double random_delay,
                                            double big_delay,
                                            double seconds_between_big_delay);

// This PacketPipe emulates network outages. It basically waits
// for 0-2*|average_work_time| seconds, then kills the network for
// 0-|2*average_outage_time| seconds. Then it starts over again.
scoped_ptr<PacketPipe> NewNetworkGlitchPipe(double average_work_time,
                                            double average_outage_time);

// This method builds a stack of PacketPipes to emulate a reasonably
// good wifi network. ~20mbit, 1% packet loss, ~3ms latency.
scoped_ptr<PacketPipe> WifiNetwork();

// This method builds a stack of PacketPipes to emulate a
// bad wifi network. ~5mbit, 5% packet loss, ~7ms latency
// 40ms dropouts every ~2 seconds. Can reorder packets.
scoped_ptr<PacketPipe> BadNetwork();

// This method builds a stack of PacketPipes to emulate a crappy wifi network.
// ~2mbit, 20% packet loss, ~40ms latency and packets can get reordered.
// 300ms drouputs every ~2 seconds.
scoped_ptr<PacketPipe> EvilNetwork();

}  // namespace test
}  // namespace cast
}  // namespace media

#endif  // MEDIA_CAST_TEST_UTILITY_UDP_PROXY_H_
