use crystal_sphinx::plugin;
use crystal_sphinx_editor::run;
use engine::utility::VoidResult;

fn main() -> VoidResult {
	run(plugin::Config::default().with(vanilla::Plugin::default()))?;
	Ok(())
}
