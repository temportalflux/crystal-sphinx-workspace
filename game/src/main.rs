use crystal_sphinx::{plugin::Config, Runtime};

fn main() {
	engine::register_thread();
	let plugins = Config::default().with(vanilla::Plugin::default());
	engine::run(Runtime::new(plugins));
}
