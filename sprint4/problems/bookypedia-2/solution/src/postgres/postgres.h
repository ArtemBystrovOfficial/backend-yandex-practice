#pragma once
#include <pqxx/connection>
#include <pqxx/transaction>

#include "../domain/author.h"
#include "../domain/book.h"

namespace postgres {

class AuthorRepositoryImpl : public domain::AuthorRepository {
public:
    explicit AuthorRepositoryImpl(pqxx::work& worker)
        : worker_{worker} {
    }

    void Save(const domain::Author& author) override;
    list_authors_t GetList() override;

private:
    pqxx::work& worker_;
};

class BookRepositoryImpl : public domain::BookRepository {
public:
    explicit BookRepositoryImpl(pqxx::work& worker)
        : worker_{worker} {        
    }

    void Save(const domain::Book& book) override;
    list_books_t GetList() override;
    list_books_t GetBookByAuthorId(const domain::AuthorId &) override;

private:
    pqxx::work& worker_;
};

class Database {
public:
    explicit Database(pqxx::connection connection);

    pqxx::connection & GetConnection() { return connection_; }

private:
    pqxx::connection connection_;
};

}  // namespace postgres