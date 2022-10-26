use crystal_sphinx_editor::Runtime;

fn main() {
	engine::register_thread();
	engine::run(Runtime::new());
}
