
WebServer DoorbellWebServer::server(80);

StaticJsonDocument<1024> jsonDocument;
char buffer[1024];

void handlePost() {
    """
    Handle a POST request send to this server.
    """

    // TODO: Enter the `post data name` variable name.

    if (server.hasArg("post data name") == false) {
        // Handle error, or ignore. No data sent. If that is the case.
    }

    // Retrieve the posted json data from the request.
    String body = server.arg("post data name");
    // Deserialise the json data into `jsonDocument` variable.
    deserializeJson(jsonDocument, body)

    // Respond, maybe change this to be a more simple ok response to reduce bandwidth requirement.
    server.send(200, "application/json", "{}");
}

void createJson(/* TODO: Add necessary parametrs to this function. */) {
    int _ = 0;

    jsonDocument.clear();
    jsonDocument["variable name"] = _; // variable value.
    jsonDocument["variable name"] = _; // variable value.
    jsonDocument["variable name"] = _; // variable value.

    serializeJson(jsonDocument, buffer);
}

void addJsonObject(/* TODO: Add necessary parameters to this function. */) {
    int _ = 0;

    JsonObject obj = jsonDocument.createNestedObject();
    obj["variable name"] = _; // variable value.
}

void getValues() {
    
}
