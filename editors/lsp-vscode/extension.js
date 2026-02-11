const path = require("path");
const { workspace } = require("vscode");
const { LanguageClient, TransportKind } = require("vscode-languageclient/node");

let client;

function activate(context) {
    const config = workspace.getConfiguration("c-xrefactory");
    const configuredPath = config.get("executablePath");

    const serverPath = configuredPath
        ? configuredPath
        : path.join(context.extensionPath, "..", "..", "src", "c-xref");

    const serverOptions = {
        command: serverPath,
        args: ["-lsp"],
        transport: TransportKind.stdio,
    };

    const clientOptions = {
        documentSelector: [{ scheme: "file", language: "c" }],
    };

    client = new LanguageClient(
        "c-xrefactory-lsp",
        "c-xrefactory",
        serverOptions,
        clientOptions,
    );

    client.start();
}

function deactivate() {
    if (client) {
        return client.stop();
    }
}

module.exports = { activate, deactivate };
