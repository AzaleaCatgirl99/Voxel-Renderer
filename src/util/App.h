#pragma once

struct AppProperties {
    const char* m_name;
    const char* m_version;
    const char* m_identifier;
    const char* m_author;
    const char* m_license;
    const char* m_homepage;
    const char* m_type;
};

class App final {
public:

private:
    AppProperties m_properties;
};
