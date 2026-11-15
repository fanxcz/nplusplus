const vscode = require('vscode');
const { LanguageClient } = require('vscode-languageclient/node');

let client;
let outputChannel;

function activate(context) {
    const serverPath = vscode.workspace.getConfiguration('npp').get('serverPath', 'npp');
    outputChannel = vscode.window.createOutputChannel('N++ Language Server');

    const serverOptions = {
        command: serverPath,
        args: ['lsp'],
        options: {
            env: process.env,
            // The portable Windows entry point is a .cmd launcher.
            shell: process.platform === 'win32'
        }
    };

    const clientOptions = {
        documentSelector: [{ scheme: 'file', language: 'npp' }],
        synchronize: {
            fileEvents: vscode.workspace.createFileSystemWatcher('**/*.npp')
        },
        outputChannel,
        traceOutputChannel: outputChannel
    };

    client = new LanguageClient(
        'nppLanguageServer',
        'N++ Language Server',
        serverOptions,
        clientOptions
    );
    context.subscriptions.push(outputChannel, client);
    context.subscriptions.push(vscode.commands.registerCommand('npp.showServerOutput', () => {
        outputChannel.show(true);
    }));
    return client.start();
}

async function deactivate() {
    if (client) await client.stop();
}

module.exports = { activate, deactivate };
