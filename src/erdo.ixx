export module erdo;

export import :meta;
export import :witchy;
export import :calculator;
export import :xml;



extern "C++" int main(int argc, char* argv[])
{
    auto executable_path = std::filesystem::absolute(std::filesystem::path(argv[0]));
    auto xml_data_directory = executable_path.parent_path() / "xml_data";

    // test1();
    witchy::run_witchy(
        "F:/Programme/Steam/steamapps/common/ELDEN RING/Game",
        "C:/Users/Paul/Downloads/WitchyBND-v3.0.0.1-win-x64/WitchyBND.exe",
        xml_data_directory
    );

    return 1;
}