#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <filesystem>
#include <ftxui/util/ref.hpp>
#include <string>

namespace fs = std::filesystem;
using namespace ftxui;

Component customFileReader(std::vector<std::string> DATA, std::string PATH, const int _dim_x_, const int _dim_y_) {
	//manages the float values used for positioning the display
	class floatCTRL {
		public:
			floatCTRL(std::size_t screen_dim, std::size_t content_len = 0)
			: step((content_len == 0)?0.1f:1.0f/std::move(content_len)) //0.1f if content_len is 0 
			{
				if (content_len == 0) {min = 0.0f; max = 1.0f;}
				else {
					//the minimum value is obviously unstable. previous approach is (step*screen_dim)/2 - (content_len % 2 != 0)?0:step/2)
					//removed the check and currently always reducing by half a step? Maybe it should be half a step reduced while even, rather than the previous while odd condition
					min = ((step * screen_dim)/2) - (step/2);
					max  = 1-(min)-(step);
				}
				float_val = min;
			}
			//returns the current, unadjust float
			const float get() { return float_val; }
			//walks the specified amount of steps to adjust the float_val, within the bounds
			void walk(int steps) { this->float_val = bound(this->float_val +  (this->step*steps)); }
			//returns relative progress as float
			const float prog() { return (float_val - min)/(max-min); }
		//FUNCTOINS
		private:
			//returns input if it is in range, otherwise returns upper/lower boundary respectively
			float bound(float input) const { return (std::min(max, std::max(min, input))); }
		//VARIABLES
		private:
			//the actual current float_val
			float float_val;
			//minimum float value
			float min;
			//maximum float value
			float max;
			//size of a single step of content
			const float step;
	};
	//change from std::vector<std::string> DATA to Elements. Allow for vector as an alternate constructor
	class contentBase : public ComponentBase {
		public:
			// constructor
			explicit contentBase(std::vector<std::string> DATA, std::filesystem::path PATH, const int _dim_y_, const int _dim_x_)
				: _DATA(std::move(DATA)),
				_PATH(std::move(PATH)),
				float_x(_dim_x_),
				float_y(_dim_y_-4, _DATA.size()),
				dim_x(_dim_x_), dim_y(_dim_y_)
			{
				loadData();
			};
			//
			Element Render() override {
				return vbox({
					hbox({
						text("<PATH: " + _PATH.relative_path().string() + ">")
						| flex_grow
						| vcenter,
						separatorEmpty(),
						gaugeRight(float_y.prog()) | size(Direction::WIDTH, Constraint::GREATER_THAN, 20) | flex
					}),
					separator(),
					vbox(baseComp->Render())
					| focusPositionRelative(float_x.get(), float_y.get())
					| vscroll_indicator
					| frame
				});
			};
			//
			bool OnEvent(Event event) override {
				//ARROWKEYS - NAV
				if (event == Event::ArrowUp) 		float_y.walk(-1);
				else if (event == Event::ArrowDown) 	float_y.walk(1);
				else if (event == Event::ArrowLeft) 	float_x.walk(-1);
				else if (event == Event::ArrowRight) 	float_x.walk(1);
				else if (event.is_mouse()) {
					auto mouse = event.mouse();
					if (mouse.button == Mouse::WheelDown) float_y.walk(2);
					if (mouse.button == Mouse::WheelUp) float_y.walk(-2);
				}
				else return false;
				return true;
			};
			//
		private:
			//returns the count of places(digits) in a number
			std::size_t countPlaces(std::size_t num) const {
				std::size_t total_places = 0;
				while(num != 0) {
					num /= 10;
					total_places++;
				} return total_places;}
			
			//might be better to do this by reference? Reduce mem usage and what-not
			std::string handleIndent(std::string sub_string) const {
				auto subPos = sub_string.find("\t", 0);
				while (subPos != std::string::npos && (subPos+1) < sub_string.length()) {
					sub_string.replace(subPos,1,"    "); //4 space indent
					subPos = sub_string.find("\t", subPos+1);
				}
				return sub_string;
			}

			void loadData() {
				baseComp = Container::Vertical({});
				//
				std::size_t total_places = countPlaces(_DATA.size());
				//tmp var that is changed again later
				for (auto& str : _DATA) {
					//get row num
					std::size_t row_num = baseComp->ChildCount()+1;
					Elements flex_string;
					//
					std::size_t lastPos = 0;
					std::size_t strPos = 0;//str.find(" ", lastPos);
					if (strPos != std::string::npos and str.length() > 0)
						do {
							if (lastPos > str.length()) break;
							//find the beginning of the next work
							strPos = str.find(" ", lastPos);
							//if there are no more words, moves the strPos to the end of the str the final portion of the string
							if (strPos == std::string::npos) strPos = str.length();
							//the word in the string
							auto sub_string = handleIndent(str.substr(lastPos, strPos-lastPos));
							if (sub_string.length() > 0) {
								//the amount of times this string would fit across the component
								int cnt = sub_string.length() / dim_x; 
								if (cnt > 0) {
									std::size_t pos_marker = 0;
									for (int x = 0; x < cnt; x++) {
										//gets the length of string to be used
										std::size_t len = std::min(dim_x, sub_string.length()-(dim_x*x));
										//emlaces the sub string inside an element in the flex_string
										flex_string.emplace_back(text(sub_string.substr(pos_marker,len)));
										//adjust position to represent where we will read from next
										pos_marker += len;
									}
								} else flex_string.emplace_back(text(sub_string));

							}
							lastPos = strPos + 1;
						} while(strPos != std::string::npos);
					else flex_string.emplace_back(text(std::move(str)));
					//
					Component tmpComp = Renderer([&, row_num, total_places, flex_string]{
						return hbox({
							//if any string is added to this entry please adjust the SIZE parameter to match
							vbox(
								text(std::to_string(row_num) + "|")
								| bold | color(Color::Cyan) 					//before all adjustments
								| align_right							//comes before size
								| size(Direction::WIDTH, Constraint::EQUAL, total_places+1), 	//after all adjustments
								ftxui::emptyElement() | flex_grow
							),
							flexbox(flex_string,
									FlexboxConfig().SetGap(1, 1)
											.Set(FlexboxConfig::Wrap::Wrap)
											.Set(FlexboxConfig::Direction::Row)
											.Set(FlexboxConfig::AlignContent::FlexStart)
											.Set(FlexboxConfig::JustifyContent::FlexStart)),
						});
					});
					baseComp->Add(tmpComp);
				}
			}
			//
			std::vector<std::string> _DATA;
			Component baseComp;
			std::filesystem::path _PATH;
			//
			std::size_t dim_x, dim_y;
			//
			floatCTRL float_x, float_y;
	};
	return Make<contentBase>(std::move(DATA), std::move(PATH), std::move(_dim_y_), std::move(_dim_x_));

};
//returns a vector with all lines of the file
std::vector<std::string> getFileContents(std::filesystem::path file_path) {
	std::fstream file_stream(file_path);
	std::vector<std::string> file_contents;
	if (file_stream.is_open()) {
		std::string text;
		while (getline(file_stream, text)){
			file_contents.push_back(text);
		}
	}
	file_stream.close();
	return std::move(file_contents);
}
int main(int argc, char** argv) {
	//default size
	auto comp_dim_y = 40;
	auto comp_dim_x = 80;
	//filePath
	if (argc < 2) return EXIT_FAILURE;
	//if size parameters are passed
	if (argc >= 4) {
		try {
			comp_dim_y = std::stoi(argv[2]);
			comp_dim_x = std::stoi(argv[3]);
		} catch(std::exception &e) {
			//endl to flush stream on failure
			std::cout << "ERR: settings screen dimensions:\n" << e.what() << std::endl;
			return EXIT_FAILURE;
		}
	}
	//get file path
	fs::path inP(argv[1]);
	if (!std::filesystem::is_regular_file(inP)) return EXIT_FAILURE;
	auto file_contents = getFileContents(inP);
	//
	//compensating for the borders and stuff
	comp_dim_y = std::min(comp_dim_y, (int)file_contents.size() + 4 );
	auto mainComp = customFileReader(std::move(file_contents), std::move(inP), comp_dim_x, comp_dim_y);
	mainComp |= border
		| size(Direction::HEIGHT, Constraint::LESS_THAN, comp_dim_y)
		| size(Direction::WIDTH, Constraint::LESS_THAN, comp_dim_x);
	//
	auto renderer = Renderer(mainComp, [&]{
		return hbox({
			emptyElement() | flex_grow,
			mainComp->Render() | center,
			emptyElement() | flex_grow,
		});
	});
	//
	auto screen = ScreenInteractive::FitComponent();
	auto roh = CatchEvent([&screen](Event event) {
		if (event == Event::Character('q') or event == Event::Escape) {
			screen.Exit();
			return true;
		}
		return false;
	});
	screen.Loop(renderer | roh);
	screen.ExitLoopClosure(); //Unsure if this is necessary, treating it as a good luck charm 
	return EXIT_SUCCESS;
}
/*TODO:
 * Allow for vim-like line jumping. E.g. typing `:60` will move the screen to line 60
 * Allow for sed finding? Or just basic regex/rapidfuzz so that stuff can be found easier in the doc. Maybe grep?
 * Add command parsing
 * Add a config that contains default values(like the size of the window) or changes the color scheme
 * Adjust `screen` such that it fits within the terminal boundaries(check if ftxui will provide the maximum possible dimension based on terminal dimensions)
*/
