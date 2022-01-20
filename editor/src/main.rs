use crystal_sphinx::plugin;
use crystal_sphinx_editor::run;

fn main() {
	engine::run(|| run(plugin::Config::default().with(vanilla::Plugin::default())));
}
