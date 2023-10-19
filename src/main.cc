#include <fstream>
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>
#include <filesystem>
#include <ftxui/util/ref.hpp>
#include <ios>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;
using namespace ftxui;

Component customFileReader(std::string PATH, const int _dim_x_, const int _dim_y_) {
	class fileHandler {
		public:
			fileHandler(fs::path file_path, size_t visible_rows = 40, size_t buffer_rows = 120)
				: file(file_path, std::ios_base::in),
				buffer_rows(buffer_rows),
				visible_rows(visible_rows)
			{
				//Check file validity
				if (not file.good()) throw std::runtime_error("fileHandler::file.good() == false.");
				file.open(file_path, std::ios_base::in);
				//Load data
				if (file.is_open()) {
					//contains the number of the buffer entry we are starting from
					std::string line_to_get;
					while (Buffer.size() < buffer_rows)
						if (read_line_to_buffer() == false) break; //breaks if something goes wrong
				}
				return;
			}
			std::vector<std::string> get_visible(size_t size = 40) {
				std::vector<std::string> retVal(size);
				for (size_t a =0; a < 40; a++) {
					retVal.emplace_back(Buffer.front());
					Buffer.pop();
				}
				return retVal;
			}
		private:
			size_t buffer_rows;
			size_t visible_rows;
			std::queue<std::string>Buffer; //the buffer containing the lines of the file that we find important
			std::ifstream file; //input filestream
			//bool readChunk(size_t chunkSize) {
			//	//buffer_entry : the entry we begin writing into.
			//	for (int i = 0; i < chunkSize; i++) {
			//		Buffer.pop();
			//		read_line_to_buffer();
			//	}
			//	return 1;
			//}
			bool read_line_to_buffer() {//, std::ifstream::pos_type file_pos) {
				if (not file.is_open()) return 0; //failure; cannot read from file.
				std::string line_to_get;
				getline(file, line_to_get);
				Buffer.push(line_to_get);
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
				int count = 0;
				for (auto& str : file_contents.get_visible()) {
					count++;
					Component tmpComp = Renderer([&, str, count]{
						return hbox({
							text(std::to_string(count) + ":"), text(str),
						});
					});
					//
					baseComp->Add(tmpComp);
				}
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
