***Megatron***

**Service-oriented framework in C++**

Let's consider a typical situation in which many fairly experienced programmers find themselves.
We need to make a timer.

They do it this way: create a new thread, throw a lambda function into it, wait for a timeout in the thread and execute the lambda.

What are the consequences of such an action?

1. Lambda is executed in another thread.
2. All variables that are shared by the class from which the timer was called must be protected with mutexes.
3. Then you need to fight deadlocks.
As a result, for the sake of implementing a trivial timer, we created such a horror in the code that it is almost impossible to fix.
And first we will do it without mutexes. It will work without load. Under load - fall.
We then try to protect ourselves with mutexes from simultaneous access from two threads.
It still falls and hangs on deadlocks. Next we fight deadlocks. All this time, the state of the project remains to be slightly corrected, but in reality it can be safely thrown away.
Is the game worth the candle?
Isn't it easier to immediately apply a reasonable architectural approach so that the timer does not create new threads?
But besides the timer, there are many more useful things, but it’s absolutely impossible to do without a timer.

**Solution**

A service is a module that has an event processing function.

The service sends a request to the timer service like give me an alert in a second.

The request to the timer is placed in the event queue, removed from it and processed by the service.

Then the timer sends a backroute event to the consumer service and this event also gets into the event queue and is then processed in handleEvent.

What do we have? Each service has only one mutex on the event queue and locks on push and pop. This is much easier to do than blocking the use of absolutely all class members.
handleEvent runs in one thread, so there is no need to protect it with a mutex.

*What can Megatron do?*

Compile it and in the ``build/bin`` subdirectory find ``test_http``

It runs the apach benchmark on the megatron web server.

Result: 270 thousand requests per second (currently 330k). Compare with nginx - 195 thousand.
We think what's wrong, our server produces text from memory, and nginx - from disk. We make sure that the same file is returned from disk - we have a result of 197.5 thousand requests.
It works faster than nginx, all other things being equal, by a little more than 1%. And this is in C++!
Even if the software is 20% slower than Nginx, then this is already a good result, but here it is faster.

**RPC**

Megatron can transmit events over the network to another node. The client-server architecture is built using 2 calls - sendEvent(destaddr....), and on the server - passEvent() - send via the reverse route.

We just take it and implement it in 5 minutes. Moreover, if we created a cloud and the event passed through several nodes, then it will return via the reverse route. Moreover, you can send an event from the GUI window class,

if the class is inherited from one class and the response is also returned to this GUI class.

You can spend half an hour implementing a display to show some hardware parameters that are on the embedded device.

*In build/bin there is a test test_http_rpc*

This is an HTTP server that accepts a request, then makes a request to another node, waits for a response and then makes a response to the initially received HTTP request.
Rate the speed, it showed me 117 thousand requests per second.


That is, we have that in Megatron the broker is linked with handleEvent and there is no need to waste time pulling events over the network from the broker, like apache kafka.



**Other goodies**

Telnet service. Any service can order console commands from this service, which will be sent to the customer when the user enters it on the console.
This way you can organize the management of the backend or just view some of the internals.
Much like on various devices.
In telnet, help is automatically organized from all registered commands with their description and division into directories.

The telnet test is called test_telnet. When you start, the server starts, and also the telnet client is launched on it from the inside and you can play around.

Sources here: modules/telnet/demo



Plugin system

So far we have looked at standalone tests. All services are linked into them, services are registered manually,
even the config file is embedded in the code.
This is so that you don’t have to set up the test every time.

Megatron's standard operating mode is plug-in. A plugin is a binary of one or more services.
Executed as a dynamic dll library.
Look at the service code, for example modules/socket
socket.cpp
registerModule - this function is executed when the dll is loaded and as a parameter
goes iUtils - this is a pointer to all megatron global data.
Service performs actions to register itself in the service factory, and also makes it clear to Megatron that
that such and such services are contained in this plugin.
If an event arrives over the network to this service, the system loads the plugin and sends the startService event to the service,
on which various initializations are made, such as setting timers, etc.


Do make install and you will see that Megatron is installed in the ~/bin-10100 directory

It contains folders - bin - there are exe files there.
try to run mt-main-10100.exe

We go to the ../conf directory and see mt-main-10100.conf there

Back in bin.
Making a symlink mt-main-10100.exe mt-main-10100-web.exe

Run mt-main-10100-web.exe

We look in the conf, mt-main-10100-web.conf has appeared
That is, for each identical binary we received 2 different configs. Essentially these are 2 different applications. Their launcher is identical in binary, but the configs are different, so they will work differently.
In the service model, the functionality of the application is determined by the set of services that have started. The launcher itself is identical for all nodes.

Let's look at the configs


pay attention to ``Start=RPC``

Edit the line: ``Start=RPC,prodtestServerWeb``

This means that after launching Megatron, we forcibly start 2 services.
The rest of the services start automatically, pushing elbows (receiving events). The services seem to push each other and start.

Why do you need to run prodtestServerWeb manually? We need it to bind to the port and wait for a connection,
It will not be possible to push him from the outside.

Literal service names are defined in the service registration function.

``
iUtils->registerService(ServiceEnum::prodtestServerWeb,
 prodtestWebServer::Service::construct, "prodtestServerWeb");
``

**More about registering the service**

There are 2 registrations.

1. PluginInfo - megatron scans the plugins directory, ``dl_opens`` them and if there is a registerModule function, calls it with the file path parameter. Then it does dlclose.
 When a message arrives at this service, Megatron knows that this service is located in this file. It loads it and calls registerModule where pn is nullptr.

2. RegisterService is called, the service is registered and starts. The startService event arrives at the event handler. The service name that is used in the config is
what we indicated during registration.

After the service has started, it lacks some parameters and adds a list of them with default values ​​to the config file.
This way you can simply make a config, changing the created parameters to the required ones. Node addresses, ports and everything you need can be changed.

IPv6 support is available, binding addresses or remote hosts can be specified separated by commas, you can set the port to ipv6 and ipv4.


Why is this config format used ``SocketIO_maxOutBufferSize=8388608`` and not yaml?

The point is that sorting is possible; the fact that the name of the parameter is preceded by the name of the service is not so important; for me personally, this method of specifying a parameter is more obvious and there is no need to guess anything.

In the function for reading a config parameter from the service code, you need to specify a line of description for the parameter,
it is automatically displayed in the config before the parameter and after sorting it loses its meaning.

In the service codes you can see the following file eventgen.sh. What it is? This is an automatic collection of information about events that the service uses. For what?
If we received an event from the network, then for processing the event must be registered in the system, otherwise there will be an error. Until you register all events,
you'll have to test and catch unregistered ones. The same event can be registered by another service and you will not catch the error, but somewhere in the production you will catch it.
Therefore, the decision arose to register all events that occur in this code.
eventgen.sh scans the code and compiles a header that contains an event registration function that needs to be called near the call to the service registration function.
Inside the script there is a reference to the binary; you need to build it manually.

**Create our own service**

Copy-paste some ready-made service inherited from ListenerBuffered1Thread. This is essential if you don’t want to bother with multithreading inside the service.
Next, we change the names and create our own functionality.
We specified it in the config in the Start field and launch Megatron.
Examples of configs are hardcoded in the tests, you can see more there.

**Cloud**

Megatron allows you to create a hierarchical cloud of nodes using the dfsReferrer service. There are tests for creating a cloud, where you can see the logic.
The dfsCaps service is also used there, but it is now simplified, the geoIP functionality has been removed from it. The choice of uplink is carried out randomly.



PS. The victory over nginx is dedicated to the programmers who made the Buran space shuttle.
They did something on the IBM 360 of 1964 that they cannot do now, with computers a million times more powerful. I tried to find and repeat their logic.
In principle, the logic of Megatron somewhat repeats the logic of the DRAGON language, only without the graphic part,
but with the help of megatron you can implement activity diagrams almost in a visual form.



