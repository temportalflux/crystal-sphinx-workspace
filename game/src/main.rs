use crystal_sphinx::{engine::utility::VoidResult, plugin, run};

fn main() -> VoidResult {
	run(plugin::Config::default().with(vanilla::Plugin::default()))?;
	Ok(())
}
