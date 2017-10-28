#include "MongoWriter.h"
#include "MongoConv.h"
#include <algorithm>
#include <bsoncxx/json.hpp>
#include <bsoncxx/builder/stream/array.hpp>
#include <bsoncxx/builder/stream/document.hpp>
#include <bsoncxx/builder/stream/helpers.hpp>
#include <bsoncxx/types.hpp>
#include <mongocxx/instance.hpp>
#include <mongocxx/client.hpp>

using bsoncxx::builder::stream::close_array;
using bsoncxx::builder::stream::array;
using bsoncxx::builder::stream::close_document;
using bsoncxx::builder::stream::document;
using bsoncxx::builder::stream::finalize;
using bsoncxx::builder::stream::open_array;
using bsoncxx::builder::stream::open_document;
using bsoncxx::builder::stream::open_document;

#include <threading/formatters/Ascii.h>
#include <threading/SerialTypes.h>

using namespace logging;
using namespace writer;
using namespace threading;
using namespace plugin::OCMDev_MongoDBWriter;
using namespace threading;
using namespace formatter;

MongoDB::MongoDB(WriterFrontend *frontend) :
    WriterBackend(frontend),
    formatter(std::make_unique<const formatter::Ascii>(
                this, formatter::Ascii::SeparatorInfo()
                )) {}

    bool MongoDB::DoInit(const WriterInfo &info, int num_fields,
            const Field *const *fields) {
        if (BifConst::LogMongo::debug && false) {
            std::cout << "[logging::writer::MongoDB]" << std::endl;
            std::cout << "  path=" << info.path << std::endl;
            std::cout << "  rotation_interval=" << info.rotation_interval << std::endl;
            std::cout << "  rotation_base=" << info.rotation_base << std::endl;

            // Output the config sorted by keys.
            std::vector<std::pair<string, string> > keys;

            for (const auto &i : info.config)
                keys.emplace_back(i.first, i.second);

            std::sort(keys.begin(), keys.end());

            for (std::vector<std::pair<string, string> >::const_iterator i = keys.begin(); i != keys.end(); i++)
                std::cout << "  config[" << (*i).first << "] = " << (*i).second << std::endl;

            for (int i = 0; i < num_fields; i++) {
                const Field *field = fields[i];
                std::cout << "  field " << field->name << ": "
                    << type_name(field->type) << std::endl;
            }

            std::cout << std::endl;
        }
        mongocxx::instance instance{};
        return true;
    }

bool MongoDB::DoWrite(int num_fields, const Field *const *fields, Value **vals) {
    bool success = false;
    string tag;

    mongocxx::uri uri("mongodb://localhost:27017");
    mongocxx::client client(uri);
    mongocxx::database db = client["mydb"];
    mongocxx::collection coll = db["conn"];

    auto builder = bsoncxx::builder::stream::document{};

    for (int i = 0; i < num_fields; i++) {
        addField(builder, fields[i], vals[i]);
    }

    // End packet
    bsoncxx::document::value doc_value  =
        builder << bsoncxx::builder::stream::finalize;

    //Aquire RO copy
    bsoncxx::document::view view = doc_value.view();

    //Place packet into collection
    bsoncxx::stdx::optional<mongocxx::result::insert_one> result 
        = coll.insert_one(view);

    return true;
}

bool MongoDB::DoSetBuf(bool enabled) {
    return true;
}

bool MongoDB::DoRotate(const char *rotated_path, double open, double close, bool terminating) {
    if (!FinishedRotation("/dev/null", Info().path, open, close, terminating)) {
        Error(Fmt("error rotating %s", Info().path));
        return false;
    }

    return true;
}

bool MongoDB::DoFlush(double network_time) {
    return true;
}

bool MongoDB::DoFinish(double network_time) {
    return true;
}

bool MongoDB::DoHeartbeat(double network_time, double current_time) {
    return true;
}
