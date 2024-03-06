// main.cpp

#include <iostream>
#include <pqxx/pqxx>
#include <boost/json.hpp>

using namespace std::literals;
// libpqxx использует zero-terminated символьные литералы вроде "abc"_zv;
using pqxx::operator"" _zv;

using namespace boost::json;

int main(int argc, const char* argv[]) {

    try {
        if (argc == 1) {
            std::cout << "Usage: db_example <conn-string>\n"sv;
            return EXIT_SUCCESS;
        } else if (argc != 2) {
            std::cerr << "Invalid command line\n"sv;
            return EXIT_FAILURE;
        }

        pqxx::connection conn{argv[1]};
        
        pqxx::work w(conn); 

        w.exec(
            "CREATE TABLE IF NOT EXISTS books ( id SERIAL PRIMARY KEY," 
                                                "title varchar(100) NOT NULL," 
                                                "author varchar(100) NOT NULL,"
                                                "year integer NOT NULL,"
                                                "ISBN char(13) UNIQUE );"_zv);
        w.commit();

        constexpr auto tag_movie_type_1 = "movie_type_1"_zv;
        conn.prepare(tag_movie_type_1,"INSERT INTO books VALUES (DEFAULT, $1, $2, $3, $4)");

        std::string request;
        while (true) {
            pqxx::work local_work(conn); 

            std::getline(std::cin, request);
            auto json_request = parse(request);
            std::string target = json_request.at("action").as_string().c_str();

            std::string json_result = "";

            if(target == "add_book") {
                try {
                auto json_args = json_request.at("payload");
                std::string title = json_args.at("title").as_string().c_str();
                std::string author = json_args.at("author").as_string().c_str();
                int year = json_args.at("year").as_int64();
                std::optional<std::string> ISBN;
                if(!json_args.at("ISBN").is_null()) 
                    ISBN = json_args.at("ISBN").as_string().c_str();

                local_work.exec_prepared(tag_movie_type_1, title, author, year, ISBN);
                local_work.commit();
                } catch(const std::exception& e) {
                    json_result = "{\"result\":false}"s;
                }
                if(json_result.empty())
                    json_result = "{\"result\":true}";
            }
            if(target == "all_books") {
                auto result = local_work.exec("SELECT id, title, author, year, ISBN FROM books ORDER BY year DESC, title ASC, author ASC, ISBN ASC;");
                std::string json_collection = "["s;
                for(auto row : result) {         
                    auto [id, title, author, year, ISBN] = row.as<int, std::string, std::string, int, std::optional<std::string>>();
                    value json_row = {{"id"s,id},{"title"s,title},{"author"s,author},{"year",year}};
                    
                    object json_object = json_row.as_object();
                    if(ISBN)
                        json_object["ISBN"s] = *ISBN;
                    else 
                        json_object["ISBN"s] = nullptr;
                    json_row = std::move(json_object);    

                    std::stringstream ss;
                    ss << json_row;
                    json_collection += ss.str() + ","s;
                }
                if(json_collection.size() != 1)//[
                    json_collection.pop_back();

                json_collection+="]"s;

                json_result = json_collection;
            }
            if(target == "exit") {
                break;
            }
            std::cout << json_result << std::endl;
        } 
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }
    
}