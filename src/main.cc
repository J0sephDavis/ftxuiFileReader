#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <filesystem>
#include <ftxui/util/ref.hpp>
#include <string>

namespace fs = std::filesystem;
using namespace ftxui;

Component customFileReader(std::vector<std::string> DATA, std::string PATH, const int _dim_x_, const int _dim_y_) {
	class rowNumber {
		public:
			rowNumber(std::size_t largest_number)
				: places(countPlaces(largest_number))
			{/* <-_-> */}
			std::string pad(std::size_t num) {
				std::string retVal = "";
				//total spaces to add
				auto places_to_pad = 0;
				places_to_pad = places - ((num==0)?1:countPlaces(num));
				for (auto x = 0; x < places_to_pad; x++) retVal += " ";
				return std::move(retVal);
			}
			//total places in base 10
			std::size_t places = 0;
			//-----
			std::size_t countPlaces(std::size_t num) {
				std::size_t total_places = 0;
				while(num != 0) {
					num /= 10;
					total_places++;
				}
				return total_places;
			}

	};
	class contentBase : public ComponentBase {
		public:
			explicit contentBase(std::vector<std::string> DATA, std::filesystem::path PATH, const int _dim_y_, const int _dim_x_)
				: _DATA(std::move(DATA)),
				_PATH(std::move(PATH)),
				_dim_x_(std::move(_dim_x_)),
				_dim_y_(std::move(_dim_y_)){
				loadData();
			};
			Element Render() override {
				return vbox({
					hbox({
						text("<PATH: " + _PATH.relative_path().string() + ">")
						| flex_grow
						| vcenter,
						separatorEmpty(),
						gaugeRight(currentSTEP) | size(Direction::WIDTH, Constraint::GREATER_THAN, 20) | flex
					}),
					separator(),
					vbox(baseComp->Render())
					| focusPositionRelative(FLOAT_X, FLOAT_Y)
					| vscroll_indicator
					| frame
				});
			};
			//
			bool OnEvent(Event event) override {
				//ARROWKEYS - NAV
				if (event == Event::ArrowUp) {
					//prevents being below the float val where scrolling is visually complete
					auto ending_bound = std::min(end, FLOAT_Y - STEP);
					//prevents being above the float val where scrolling is visually complete
					auto fully_bound = std::max(begin, ending_bound);
					FLOAT_Y = fully_bound;
					update_step();
					return true;
				}
				else if (event == Event::ArrowDown) {
					//prevents being above the float val where scrolling is visually complete
					auto beginning_bound = std::max(begin, FLOAT_Y + STEP);
					//prevents being below the float val where scrolling is visually complete
					auto fully_bound = std::min(end, beginning_bound);
					FLOAT_Y = fully_bound;
					update_step();
					return true;
				}
				else if (event == Event::ArrowLeft) {
					FLOAT_X = std::max(0.0f, FLOAT_X - STEP);
					return true;
				}
				else if (event == Event::ArrowRight) {
					FLOAT_X = std::min(1.0f, FLOAT_X + STEP);
					return true;
				}
				return false;
			};
			//
		private:
			void update_step() {
				float tmp = (FLOAT_Y-begin);
				if (tmp > 0)
					currentSTEP = tmp/(end-begin);
				else
					currentSTEP = 0.0f;
			}
			Element renderLine(std::string line) {
				return text(line);
			};
			void loadData() {
				baseComp = Container::Vertical({});
				//
				auto data_size = _DATA.size();
				//used to find the totaldigits in a number
				auto total_places = rowNumber(data_size);//countPlaces(data_size);
				//tmp var that is changed again later
				for (auto& str : _DATA) {
					//consider replacing the innerds here with a passable,
					// 	std::function<Component(std::string>),
					//that will handle the building of the line entry? 
					//May allow for reusing this entire component when dealing with lists of strings
					//by writing a component builder for each use case.

					//the current row num
					auto row_num = baseComp->ChildCount();
					//
					auto tmpComp = Renderer([&, row_num]{
						return hbox({
							text(total_places.pad(row_num) + std::to_string(row_num) + "| ") | bold | color(Color::Cyan),// | size(Direction::WIDTH, Constraint::EQUAL,3),
							renderLine(str),
						});
					});
					baseComp->Add(tmpComp);
				}
				//gets the amount of focus change for one line move
				STEP = (1.0f / baseComp->ChildCount());
				//adjusted_y = (dim_y - border - border - separator - line)
				//beg. = STEP * adjusted_y
				//beg. /= 2
				begin = (STEP * (_dim_y_ - 4)/2) - STEP;
				FLOAT_Y = begin;
				//somewhat arbitrary multipliers here...requires some thinking
				end = 1-begin-STEP;//(STEP * _dim_y_) - 2.25 * STEP;
			}
			//
			float FLOAT_X = 0.0f;
			float FLOAT_Y = 0.0f;
			float STEP = 0.005f;
			int _dim_y_;
			int _dim_x_;
			float begin;
			float end;
			float currentSTEP = 0;
			//
			std::vector<std::string> _DATA;
			Component baseComp;
			std::filesystem::path _PATH;
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
	if (argc < 2)
		return EXIT_FAILURE;
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
	//consider giving an err msg
	if (!std::filesystem::is_regular_file(inP))
		return EXIT_FAILURE;
	auto file_contents = getFileContents(inP);
	//needed to cast from std::size_t to int
	int content_len = file_contents.size();
	//----------
	comp_dim_y = std::min(comp_dim_y, content_len + 4 /*compensating for the borders and stuff*/);
	auto mainComp = customFileReader(std::move(file_contents), std::move(inP), comp_dim_x, comp_dim_y);
	mainComp |= border
		| size(Direction::HEIGHT, Constraint::LESS_THAN, comp_dim_y)
		| size(Direction::WIDTH, Constraint::LESS_THAN, comp_dim_x);
	auto renderer = Renderer(mainComp, [&]{
		return hbox({
			emptyElement() | flex_grow,
			mainComp->Render() | center,
			emptyElement() | flex_grow,
		});
	});
	//
	auto screen = ScreenInteractive::FitComponent();
	//
	auto roh = CatchEvent([&screen](Event event) {
		if (event == Event::Character('q') or event == Event::Escape) {
			screen.Exit();
			return true;
		}
		return false;
	});
	screen.Loop(renderer | roh);
	return EXIT_SUCCESS;
}
/*TODO:
 * Allow for vim-like line jumping. E.g. typing `:60` will move the screen to line 60
 * Allow for sed finding? Or just basic regex/rapidfuzz so that stuff can be found easier in the doc. Maybe grep?
 * Add command parsing
 * Add a config that contains default values(like the size of the window) or changes the color scheme
*/
