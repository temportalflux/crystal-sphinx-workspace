use crystal_sphinx::{plugin, run};
use engine::utility::VoidResult;

fn main() -> VoidResult {
	run(plugin::Config::default().with(vanilla::Plugin::default()))?;
	Ok(())
}
