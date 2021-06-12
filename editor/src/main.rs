use crystal_sphinx::{engine::utility::VoidResult, plugin};
use crystal_sphinx_editor::run;

fn main() -> VoidResult {
	run(plugin::Config::default().with(vanilla::Plugin::default()))?;
	Ok(())
}
