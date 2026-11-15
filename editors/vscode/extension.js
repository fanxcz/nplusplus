const vscode = require('vscode');
const { execFile } = require('child_process');
const { LanguageClient } = require('vscode-languageclient/node');

let client;
let outputChannel;
let compilerChannel;

function activate(context) {
    const serverPath = vscode.workspace.getConfiguration('npp').get('serverPath', 'npp');
    outputChannel = vscode.window.createOutputChannel('N++ Language Server');
    compilerChannel = vscode.window.createOutputChannel('N++ Compiler');

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
    context.subscriptions.push(outputChannel, compilerChannel, client);

    const runCompiler = (args) => {
        const editor = vscode.window.activeTextEditor;
        if (!editor || editor.document.languageId !== 'npp') {
            vscode.window.showWarningMessage('N++: open a .npp file first.');
            return;
        }
        const file = editor.document.fileName;
        const compiler = vscode.workspace.getConfiguration('npp').get('compilerPath', 'npp');
        compilerChannel.show(true);
        compilerChannel.appendLine(`$ npp ${args.join(' ')} ${file}`);
        execFile(compiler, [...args, file], { env: process.env }, (error, stdout, stderr) => {
            if (stdout) compilerChannel.append(stdout);
            if (stderr) compilerChannel.append(stderr);
            if (error) {
                compilerChannel.appendLine(`exit code: ${error.code}`);
                vscode.window.showErrorMessage(`N++ ${args[0]} failed (see N++ Compiler output).`);
            } else {
                vscode.window.showInformationMessage(`N++ ${args[0]}: OK`);
            }
        });
    };

    context.subscriptions.push(
        vscode.commands.registerCommand('npp.showServerOutput', () => {
            outputChannel.show(true);
        }),
        vscode.commands.registerCommand('npp.check', () => runCompiler(['check'])),
        vscode.commands.registerCommand('npp.build', () => runCompiler(['build'])),
        vscode.commands.registerCommand('npp.fmt', () => runCompiler(['fmt'])),
        vscode.commands.registerCommand('npp.run', () => {
            const editor = vscode.window.activeTextEditor;
            if (!editor || editor.document.languageId !== 'npp') {
                vscode.window.showWarningMessage('N++: open a .npp file first.');
                return;
            }
            const terminal = vscode.window.createTerminal('N++ run');
            const compiler = vscode.workspace.getConfiguration('npp').get('compilerPath', 'npp');
            terminal.sendText(`${compiler} run "${editor.document.fileName}"`);
            terminal.show();
        })
    );
    return client.start();
}

async function deactivate() {
    if (client) await client.stop();
}

module.exports = { activate, deactivate };
