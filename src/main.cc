#include <fstream>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/component/component.hpp> //for CatchEvent()
#include <ftxui/dom/elements.hpp>
#include <sstream>
#include <vector>
#include <string>
#include <filesystem>

//TODO: allow command input, vim-style at the bottom / jump to line
//TODO: syntax-highlighting
//TODO: set float_y to the bottom-most visible row (to avoid pressing downarrow 50 times)
//TODO: set float_x to the right-most visible portion (to avoid pressing arrowRight a bunch)
//TODO: Look into reading from piped files. e.g., COMMAND | less --> COMMAND | reader should be roughly equivalent. Likely need an empty constructor + function to add element rows

namespace fs = std::filesystem;
namespace ui = ftxui;
static const std::string tab_string = "    ";
//component to display the text & render
class viewingPane : public ui::ComponentBase {
	private:
		std::vector<ui::Element> rows = {};
		float focus_x = 0.f;
		float focus_y = 0.f;
		float increment_amount;
	public:
		viewingPane(std::vector<std::string> file_contents) {
			increment_amount = 1.0f/file_contents.size();
			int number_of_digits = 0;
			{
				auto total = file_contents.size();
				while (total != 0) {
					number_of_digits++;
					total /= 10;
				}
			}
			size_t count = 1;
			for (auto &line : file_contents) {
				std::string padding = "";
				{
					size_t tmp_count = count;
					int line_numberOfDigits = 0;
					//only in effect when we start count at 0; left as a precaution
					if (tmp_count == 0)
						line_numberOfDigits = 1;
					else while (tmp_count != 0) {
						line_numberOfDigits++;
						tmp_count /= 10;
					}
					tmp_count = number_of_digits - line_numberOfDigits;
					while(tmp_count != 0) {
						padding.append(" ");
						tmp_count--;
					}
				}
				ui::Element line_item = ui::hbox({
						ui::text(padding + std::to_string(count++))| ui::bold | ui::color(ui::Color::Blue),
						ui::separator(),
						ui::text(line)
				});
				rows.push_back(line_item);
			}

		}
		bool OnEvent(ui::Event event) override {
			//prevents the float value from exceeding 1.0f or being reduced below 0.0f
			auto keep_in_bounds = [&](float value, float increment_amount, bool increase) -> float {
				if (increase)
					return (value + increment_amount > 1.0f ? 1.0f : value + increment_amount);
				else
					return (value - increment_amount < 0.0f ? 0.0f : value - increment_amount);
			};
			//
			if (event == ui::Event::ArrowDown) {
				focus_y = keep_in_bounds(focus_y, increment_amount, 1);
				return true;
			}
			if (event == ui::Event::ArrowUp) {
				focus_y = keep_in_bounds(focus_y, increment_amount, 0);
				return true;
			}
			if (event == ui::Event::ArrowRight) {
				focus_x = keep_in_bounds(focus_x, 0.1f, 1);
				return true;
			}
			if (event == ui::Event::ArrowLeft) {
				focus_x = keep_in_bounds(focus_x, 0.1f, 0);
				return true;
			}
			return false;
		}
		ui::Element Render() override {
			return ui::vbox(rows)
				| ui::vscroll_indicator
//				| ui::hscroll_indicator //not in FTXUI V5.0.0, yet.
				| ui::focusPositionRelative(focus_x, focus_y)
				| ui::frame
			;
		}
		//returns the focus_y variable
		const float getVerticalFocus() {
			return this->focus_y;
		}
};

int main(int argc, char** argv) {
	//process argument & load file content
	std::vector<std::string> file_contents;
	bool pipe_reading_warning = false;
	if (argc < 2) {
		//check to see if we are reading from the standard input
		if (!std::cin.good()) exit(EXIT_FAILURE);
		//read from pipe
		pipe_reading_warning = true;
		std::string inputLine;
		while(getline(std::cin, inputLine)) {
			auto iterator = inputLine.find('\t');
			while (iterator != std::string::npos) {
				inputLine.replace(iterator, 1, tab_string);
				iterator = inputLine.find('\t');
			}
			file_contents.push_back(inputLine);
		}
		//bring stdin back to the user
		stdin = freopen("/dev/tty", "r", stdin);
	}
	else { //TODO: Determine the proper checks to ensure the file/stream is good & readable.
		fs::path file_path(argv[1]);
		if (!fs::is_regular_file(file_path)) exit(EXIT_FAILURE);
		std::ifstream file(file_path);
		if (file.bad()) exit(EXIT_FAILURE); //TODO: see if this is necessary
		//read from file
		std::string fileLine;
		while(std::getline(file, fileLine)) {
			auto iterator = fileLine.find('\t');
			while (iterator != std::string::npos) {
				fileLine.replace(iterator, 1, tab_string);
				iterator = fileLine.find('\t');
			}
			file_contents.push_back(fileLine);
		}
		file.close();
	}
	if (file_contents.empty()) exit(EXIT_FAILURE);
	//creating the UI
	auto application_screen = ui::ScreenInteractive::FitComponent();
	auto file_viewing_screen = ui::Make<viewingPane>(std::move(file_contents));
	auto quit_handler = ui::CatchEvent([&application_screen](ui::Event event){
		if (event == ui::Event::Character('q')) {
			application_screen.ExitLoopClosure()();
			return true;
		}
		return false; //no event handled
	});
	auto scroll_renderer = ui::Renderer(file_viewing_screen, [&] {
			auto header = "Viewing Pane Renderer";
			return ui::vbox({
					ui::hbox({
						ui::text(header),
						ui::gauge(file_viewing_screen->getVerticalFocus())
					}),
					ui::separator(),
					file_viewing_screen->Render(),
			}) | ui::border;
	});
	//the main loop
	application_screen.Loop(scroll_renderer | quit_handler);
	exit(EXIT_SUCCESS);
}
