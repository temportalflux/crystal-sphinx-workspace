Singletons:

- input system
	write:
		project initializing the configuration (see boid demo)
		engine updating/sending events
	read:
		getting input data (button press or axis input) - can be in any place like in an ecs system updater
	problem:
		need access to initialize the input system with a configuration (write),
		update the inputs per tick (write) via the engine,
		and read inputs at any time (read), often on tick.
	concepts:
		- If actions were objects instead of ids, the action objects could be wrapped in arcs,
			and provided to both the input system and the consumers. This has the issue of how obtuse
			the handling of multiple actions and multiple players would become.
		- Consumers of input actions can be provided a state object for that user. It is shared RWLock with the
			system so the system can update action states within it as well as connection status (if there is input driving it).
			This would allow input system to be initialized at startup with a config and owned by the engine.
			If the input system itself is also an arc-lock, then users can grab user objects from it in the future
			(i.e. player selects the number of players at runtime).
			Player state objects would be able to retain information about their actions, and consumers would have access to both.

- asset library (tracks what pak files have been scanned and provides the binaries for reading assets into memory)
	write:
		engine::scan_paks - scan the pak directory
		editor::new - scan the pak directory
	read:
		asset::Id::has_been_scanned
		asset::Loader::load_sync(asset::Id) - loads a specific asset by id
		ui::System::with_all_fonts - loads all the font assets
	solution:
		a. See below for asset loading thread. The library can be exclusively owned by the load thread,
			and initialized once at startup.

- asset types (keeps track of what asset types have been registered so they can be used when loading assets into memory)
	write:
		engine::register_asset_types - registers module asset types
	read:
		asset::Loader::(decompile & load_sync)
		- reads binary asset from pak file as its asset type
		- this struct is all static functions that can be called at any time, so its a challenge to always have the type registry available
	problem:
		Write only happens on startup, and `Engine::new` could be refactored so that crates listen to an engine callback to register asset types.
		Read access is only needed via loader functions, but can happen at any time and in any thread.
	solutions:
		a. Move asset loading to be completely asyncrhonous
			This would both improve the loading flow of managing assets (and not block the main thread with asset loading),
			would leverage the task system, and would mean the type registry is owned by the asset loader thread.
			Could use multi-producer single-consumer channels (std::sync::mpsc or crossbeam) for sending load requests to asset thread.
			This would also enable asset-caching in a way that the asset load thread knows how assets need to be kept around
			vs which can be disposed (and requests made while an asset is still cached would almost immediately callback).
			This will, ofc, introduce the sender channel as a global of some sort so events can be sent through it.
			Hurdles:
			- asset thread needs to be able to enqueue load-finished callbacks for the main thread. Might be solved via task system?

- audio system (need to playback audio on the fly)
	write:
		engine tick update
		audio sequence to play the next track
		audio demo to control playback
	read: n/a
	notes:
		its /probably/ ok that this is a singleton because writes only happen internally (during engine tick)
		or when sending the signal to play a track. That said, it could use some refinement to load assets
		up front and then send the asset id to the system, which would be processed on a thread.

- network manager (for sending packets on the fly)
	write:
		engine tick update - process packets
		chat demo:
			initialization
			- adding network observer (for connection events)
			- starting the network
			- send packet (handshake)
			handshape processing
			- send packet to client
			ui input
			- send message to server
			message processing
			- send from server to all clients
	read:
		network::mode - get the mode of the network (client &| server)
		network::connection::Id::as_address - get the address via the network manager's connection list
	problem:
		engine update (Network::process) really only needs access to the incoming_queue and the packet type registry,
		(and then needs to know how to handle connections, but that can be done by adding a processor for those events).
		If outgoing_queue has its own object (which is the globally-accessible singleton) with access to the connection list,
		then that can be what packets send using.
	solution:
		IncomingQueue + TypeRegistry live as a bundle on the engine. If the bundle is non-None, then there is a live network.
		OutgoingQueue + a handle to IncomingQueue live as a bundle, which is mutexed globally.
		To send a packet, out-bundle needs to be locked and then written to. To close the network, you lock the
		out-bundle like you would when sending a packet, but call a `stop` function which sends the stop event through
		the incoming queue (so the network is stopped and dropped on next process).
		- Socknet::Socket should not be an object, but rather export the relevant queues.
		- There should be a crossbeam channel to extract packets from the network thread, rather than an arc-tex vecdeque.
			That unbounded crossbeam channel will be what the in-bundle will read from.

- network packet types (keeping track of what packet types are registered and how they are processed when received by the manager)
	write:
		register packet types on app initialization
	read:
		engine network manager
		- deserialize a packet by its type based on registered packets
	solution:
		broadcast a registration phase on startup, and then hand off the registry to be owned by the network manager

- editor
	write:
		Simulation::save_open_state - mark the window as open or not and save settings
	read:
		on start build - access the crates in the editor to build assets and package paks
		Simulation::new - is the simulation window open (check editor settings)
		run_commandlets instead of opening the editor ui

- editor's crate manifest
	write: n/a
	read:
		Editor::new - read the config file so the editor knows what crates/modules are being editted
	solution:
		this should just be an object owned by the editor, it should never have been a singleton

- crystal sphinx plugin manager
	write:
		on app init, load the plugin config
	read:
		register music asset-ids based on the registered plugins

- boid game context (holds channel senders for creating and destroying boids)
	write:
		app init - set the channel senders to send events to ECS systems
	read:
		ui button input - send create/destroy events to ECS via game context

- chat room message history
	write:
		message packet received - record message in the history
	read:
		ui log - display the message history as ui widgets
