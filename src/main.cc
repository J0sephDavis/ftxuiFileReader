#include <fstream>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp> //for CatchEvent()
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>

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
		file.close();
	}
	if (file_contents.empty()) exit(EXIT_FAILURE);
	//the screen that contains(is) the top-level component
	auto application_screen = ui::ScreenInteractive::FitComponent();
	//the viewing-pane to render
	ui::Component test_comp = ui::Make<viewingPane>(std::move(file_contents));
	//component decorator that will quit the current component loop
	test_comp |= ui::CatchEvent([&application_screen](ui::Event event){
		if (event == ui::Event::Character('q')) {
			application_screen.ExitLoopClosure();
			application_screen.Exit();
			return true;
		}
		return false; //no event handled
	});
	
	application_screen.Loop(test_comp | ui::border);
	exit(EXIT_SUCCESS);
}
