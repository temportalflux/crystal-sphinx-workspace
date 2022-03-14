use crystal_sphinx::{plugin::Config, Runtime};

fn main() {
	let plugins = Config::default().with(vanilla::Plugin::default());
	engine::run(Runtime::new(plugins));
}
