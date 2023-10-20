#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <ftxui/dom/elements.hpp>
#include <list>
#include <filesystem>
#include <ftxui/util/ref.hpp>
#include <ftxui/dom/table.hpp>
#include <ios>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace ftxui;

Component customFileReader(std::string PATH, const int _dim_x_, const int _dim_y_) {
	// Consider switching to structs
	class file_marker {
		public:
			file_marker(size_t line_num, size_t file_pos)
				: line(line_num), pos(file_pos)
			{}
			size_t getPos() const { return pos; };
			size_t getLine() const { return line; };
		private:
			const size_t pos;
			const size_t line;

	};
	class composite_entry {
		public:
			composite_entry(size_t line_num, std::string content)
				: _content(content),
				_line_num(line_num)
				{}
			std::string getContent() const {
				return _content;
			}
			std::size_t getLineNum() const {
				return _line_num;
			}
		private:
			const std::string _content;
			const std::size_t _line_num;
	};
	class fileHandler {
		public:
			fileHandler(fs::path file_path, size_t visible_rows = 40, size_t buffer_rows = 120)
				: file(file_path, std::ios_base::in),
				buffer_rows(buffer_rows),
				visible_rows(visible_rows)
			{
				//Check file validity
				if (file.good() and file.is_open()) {
					//declare variables
					bool readFlag = true;
					int rowCount;
					std::string line_to_get;
					//Load data
					for (rowCount = 0; rowCount < buffer_rows and readFlag; rowCount++)
						readFlag = read_line_to_buffer();
					//adjust size variables
					if (rowCount < buffer_rows) buffer_rows = rowCount;
					if (visible_rows > buffer_rows) visible_rows = buffer_rows;
				}
				return;
			}
			std::vector<std::string> get_next(size_t size) {
				std::vector<std::string> retVal;
				size_t rows = std::min(size, Buffer.size());
				for (size_t line =0; line < rows; line++) {
					retVal.push_back("[" + std::to_string(Buffer.front().getLineNum()) + "]" + Buffer.front().getContent());
					Buffer.pop_front();
				}
				return retVal;
			}
			std::vector<std::string> show_markers() {
				std::vector<std::string> retVal;
				retVal.push_back("Line,Pos [" + std::to_string(markers.size()) + "]");
				for (auto& mark : markers) {
					retVal.push_back(std::to_string(mark.getLine()) + "," + std::to_string(mark.getPos()));
				}
				return retVal;
			}
			std::vector<std::vector<std::string>> markers_as_table() {
				std::vector<std::vector<std::string>> retVal;
				retVal.reserve(markers.size()+1);
				retVal.push_back({"Line","Pos"});
				for (auto& mark : markers) {
					retVal.push_back({std::to_string(mark.getLine()), std::to_string(mark.getPos())});
				}
				return retVal;
			}
		private:
			/* Reduce the composite_entry to only contain {line_number, line_content}.
			 * Make a new vector/list that contains only {line_number, file_pos}. Sort list by either value, they should coincide.
			 * The new vector line_markers{} will be used for jumping around the file.
			 * This means if we currently have lines 400-500 in the Buffer, but we want to see 300-400...
			 * We can visit line_markers.line(300) will return the position marker for 300 and we can read from there.
			 * */
			size_t buffer_rows;
			size_t visible_rows;
			size_t row_count = 0;
			std::deque<composite_entry>Buffer; //the buffer containing the lines of the file that we find important
			std::list<file_marker>markers;
			std::ifstream file; //input filestream
			//adds marker
			void add_marker(const size_t file_pos, const size_t line_num) {
				markers.push_back({row_count, file_pos});
			}
			//Reads the next line in the file to the buffer
			bool read_line_to_buffer() {//, std::ifstream::pos_type file_pos) {
				//precondition
				if (not file.is_open()) return 0; //failure; cannot read from file.
				//function body
				std::string line_to_get;
				auto pos_before_read = file.tellg();
				getline(file, line_to_get);
				//update buffer & marker data
				if ((row_count % 10) == 0) add_marker(pos_before_read, row_count);
				Buffer.emplace_back(row_count++, line_to_get);
				//
				return 1;
			}
	};
	//change from std::vector<std::string> DATA to Elements. Allow for vector as an alternate constructor
	class contentBase : public ComponentBase {
		public:
			// constructor
			explicit contentBase(std::filesystem::path PATH, const int _dim_y_, const int _dim_x_)
				:file_contents(PATH, _dim_y_),
				_PATH(PATH),
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
						//gaugeRight(float_y.prog()) | size(Direction::WIDTH, Constraint::GREATER_THAN, 20) | flex
					}),
					separator(),
					vbox(baseComp->Render())
				});
			};
			//
			//bool OnEvent(Event event) override {
			//	//ARROWKEYS - NAV
			//	if (event == Event::ArrowUp) 		file_contents.walk(-1);
			//	else if (event == Event::ArrowDown) 	file_contents.walk(1);
			//	else return false;
			//	return true;
			//};
			//
		private:
			////returns the count of places(digits) in a number
			//std::size_t countPlaces(std::size_t num) const {
			//	std::size_t total_places = 0;
			//	while(num != 0) {
			//		num /= 10;
			//		total_places++;
			//	} return total_places;}
			//
			////might be better to do this by reference? Reduce mem usage and what-not
			//std::string handleIndent(std::string sub_string) const {
			//	auto subPos = sub_string.find("\t", 0);
			//	while (subPos != std::string::npos && (subPos+1) < sub_string.length()) {
			//		sub_string.replace(subPos,1,"    "); //4 space indent
			//		subPos = sub_string.find("\t", subPos+1);
			//	}
			//	return sub_string;
			//}

			void loadData() {
				baseComp = Container::Vertical({});
				//
				for (auto& str : file_contents.get_next(20)) {
					Component tmpComp = Renderer([&, str]{
						return hbox({
							text(str),
						});
					});
					//
					baseComp->Add(tmpComp);
				}
				auto table = Table(file_contents.markers_as_table()).Render();
				auto table_renderer = Renderer([&,table] {
					return hbox(table) | borderRounded | flex_shrink;
				});
				baseComp->Add(table_renderer);

			}
			//
			Component baseComp;
			std::filesystem::path _PATH;
			fileHandler file_contents;
			//
			std::size_t dim_x, dim_y;
			//
			//floatCTRL float_x, float_y;
	};
	return Make<contentBase>(std::move(PATH), std::move(_dim_y_), std::move(_dim_x_));

};
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
	//compensating for the borders and stuff
	comp_dim_y = comp_dim_y;
	auto mainComp = customFileReader(std::move(inP), comp_dim_x, comp_dim_y);
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
