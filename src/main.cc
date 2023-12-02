#include <fstream>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp> //for CatchEvent()
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>

//TODO: scrollbar
//TODO: line-numbers
//TODO: allow command input, vim-style at the bottom
//TODO: jump to line
//TODO: syntax-highlighting

namespace fs = std::filesystem;
namespace ui = ftxui;
//component to display the text & render
class viewingPane : public ui::ComponentBase {
	private:
		std::vector<ui::Element> rows = {};
	public:
		viewingPane(std::vector<std::string> file_contents) {
			for (auto &line : file_contents) {
				rows.push_back(ui::text(line));
			}
		}

		ui::Element Render() override {
			return ui::vbox(rows);
		}
};

int main(int argc, char** argv) {
	if (argc < 2) exit(EXIT_FAILURE);
	fs::path file_path(argv[1]);
	if (!fs::is_regular_file(file_path)) exit(EXIT_FAILURE);
	//the file the user passed
	std::vector<std::string> file_contents;
	{
		std::ifstream file(file_path);
		std::string fileLine;
		while(std::getline(file, fileLine)) {
			file_contents.push_back(fileLine);
		}
		file.close(); //likely unnecessary as it is deconstructed after this.
	}
	if (file_contents.empty()) exit(EXIT_FAILURE);
	//the screen that contains(is) the top-level component
	auto application_screen = ui::ScreenInteractive::FitComponent();
	auto quit_handler = ui::CatchEvent([&application_screen](ui::Event event){
		if (event == ui::Event::Character('q')) {
			application_screen.ExitLoopClosure()();
			return true;
		}
		return false; //no event handled
	});
	//
	float focus_x=0, focus_y=0;
	ui::Component test_comp = ui::Make<viewingPane>(std::move(file_contents));
#define increment 0.05f
	test_comp |= ui::CatchEvent([&focus_x,&focus_y](ui::Event event){
			if (event == ui::Event::ArrowDown) {
				focus_y = (focus_y + increment > 1.0f ? 1.f : focus_y + increment);
				return true;
			}
			if (event == ui::Event::ArrowUp) {
				focus_y -= (focus_y - increment < 0.f ? 0.f : focus_y - increment);
				return true;
			}
			if (event == ui::Event::Escape) {
				focus_y = 0.f;
				return true;
			}
			return false;
	});
	auto scroll_renderer = ui::Renderer(test_comp, [&] {
			auto header = "Viewing Pane Renderer x:(" + std::to_string(focus_x) + ") y:(" + std::to_string(focus_y) + ")";
			return ui::vbox({
					ui::text(header),
					ui::separator(),
					test_comp->Render()
						| ui::focusPositionRelative(focus_x, focus_y)
						| ui::frame
						| ui::vscroll_indicator,
			}) | ui::border;
	});
	application_screen.Loop(scroll_renderer | quit_handler);
	exit(EXIT_SUCCESS);
}
