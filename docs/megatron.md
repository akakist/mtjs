# Megatron

## Concept

Megatron is an implementation of a service-oriented, event-driven concept.

Unlike traditional OOP, where the primary logical unit is an object, in Megatron, it is a service. Instead of using methods to interact with objects, as in OOP, events are used here. This is also known as the actor model.

A service in Megatron typically runs as a separate shared object (so), with its code fully isolated from other services. This ensures maximum code isolation to prevent the creation of spaghetti code, where everything is intertwined, and changing one part causes issues elsewhere. Services interact with each other through events, with event code placed in headers.

Advantages of this approach for developing complex systems:
1. A single service has limited logic. Its development and testing are done using functional tests that generate fake events. For example, to debug a service that receives events from hardware, we create a fake event generator that mimics hardware events for debugging. Meanwhile, other developers can work on the actual hardware service.
2. RPC is already available, enabling events to pass over the network in a manner similar to how they work within a process. The logic changes minimally. We can debug all services within a single process, then split them across nodes with minimal adjustments.
3. In other words, this approach significantly saves time when implementing complex control systems, even at the debugging stage.

For example, the Telnet implementation is a consumer of the socket service (socket multiplexer). Telnet receives buffers of read data, parses them according to the Telnet protocol, and issues notifications when a command is entered. This is also an event that reaches the MTJS service, which implements the JS interface. Megatron’s logic aligns perfectly with the JS event loop, where everything is also built on events.

The main benefits of Megatron have already been integrated into MTJS. One of its standout features is the hierarchical cloud manager, which can be used for building instant messaging (IM), blockchain, and similar systems. The cloud accelerates content distribution. For example, to distribute a 1 MB file to 1 million users over a 100 Mbit/s network, transferring 1 TB of data would take 100,000 seconds (over a day). With a binary cloud, 1 million users correspond to 20 levels. Distributing 1 MB to one level takes 0.2 seconds, totaling 4 seconds for the entire tree.

## P2P Service for IM

Each user (mobile app) has an uplink node in the cloud. The connection to the uplink is short-lived, only when the app is active. Another user also has an uplink. Information about uplinks for each user is broadcast across the cloud, and each node maintains a database mapping `userId` to `uplinkAddr`. When an uplink receives a message addressed to another user, it finds the recipient’s uplink address, forwards the message directly, and the recipient’s uplink stores it, delivering it to the user when they connect.
