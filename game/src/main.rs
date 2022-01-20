use crystal_sphinx::{plugin, run};

fn main() {
	engine::run(|| run(plugin::Config::default().with(vanilla::Plugin::default())));
}
