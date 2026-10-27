# Novolang (NCC) for VS Code and VSCodium

Version 0.2.0. C-based syntax highlighting for `.nl` files with Novolang extensions.

## Supported syntax

- C-style functions and parameters, globals, named structs, fields, pointers, member/arrow access, indexing, control flow, comments, strings, character literals, and escapes.
- `name: Type`, `name := expression`, and `NAME :: expression` declarations.
- Aliases such as `Byte :: u8`, `PointPtr :: Point*`, and `Point :: struct { int x; };`.
- Fixed arrays (`int[4]`), `sizeof`, C-style casts, and logical operators `!`, `&&`, `||`.
- `#import "relative/file.nl"` and the `syscall(...)` builtin.
- Sized integer names are colored for editing convenience; the compiler decides which widths are implemented.

This extension extends the editor's built-in C grammar; keep C/C++ Language Basics enabled. It does not change `.c` or `.h` files. Snippets retain C-style struct fields (`int x;`) because colon-style fields have not been implemented in the compiler yet.

TextMate coloring is syntactic, not semantic. Plain identifier-to-identifier aliases and alias references in ambiguous expressions cannot always be distinguished from constants/variables. Pointer-cast patterns and explicit type positions provide type coloring without treating every `(x)` expression as a cast. Your theme decides the actual colors. There is no LSP, semantic completion, type checking, compiler diagnostics, or formatter.

## Put the source in your NCC repo

Extract the bundle at the NCC repository root. It contains `editors/vscode/` and a prebuilt `novolang-language-0.2.0.vsix`. The `editors/vscode` folder is the extension source, not something VS Code automatically installs.

Commit the extension source with NCC:

```sh
git add editors/vscode
git commit -m "Update Novolang editor syntax support"
```

## Install the prebuilt extension

From the extracted bundle directory:

```sh
code --install-extension ./novolang-language-0.2.0.vsix --force
# VSCodium alternative:
codium --install-extension ./novolang-language-0.2.0.vsix --force
```

Or use **Extensions: Install from VSIX...** in the Command Palette. Then run **Developer: Reload Window**. It keeps the original extension identifier (`novolang-local.novolang-language`) so this updates version 0.1.0.

If you already have an override assigning `.nl` to C, merge this entry into your existing settings JSON:

```json
"files.associations": {
  "*.nl": "novolang"
}
```

Do not replace your other settings. No repository `.vscode` files are overwritten by this bundle.

## Edit and preview from this repo

Run from the NCC root:

```sh
code --extensionDevelopmentPath="$PWD/editors/vscode" "$PWD"
# Or replace code with codium.
```

Open `editors/vscode/examples/highlighting.nl` in the development window. Use **Developer: Inspect Editor Tokens and Scopes** to inspect theme scopes. After editing a grammar, reload that window.

## Package future changes

Requires Node.js 22+ and npm. From the NCC root:

```sh
cd editors/vscode
npm install
npm run package
code --install-extension ./novolang-language-0.2.0.vsix --force
```

Change the version in `package.json` before distributing a new release. Local packaging/installing does not require Marketplace publication or a publisher account. `novolang-local` is just this extension's local identifier. The package remains UNLICENSED; select a license compatible with your project before publishing.

## Snippets

`main`, `fn`, `import`, `if`, `while`, `for`, `var`, `infer`, `const`, `alias`, `struct`, `array`, `sizeof`, `cast`, `syscall`, and `linux_write`.

## Compiler smoke check

The highlighting fixture covers imports, global constants/aliases, structs, local type annotations and inference, scoped for declarations, arrays/indexing, casts, sizeof, pointers, logical short-circuiting, strings, escapes, and character literals. It should exit with 42. Run from the NCC root after extracting the bundle:

```sh
mkdir -p build
./build/ncc editors/vscode/examples/highlighting.nl > build/editor-smoke.s &&
  as -o build/editor-smoke.o build/editor-smoke.s &&
  ld -o build/editor-smoke build/editor-smoke.o &&
  ./build/editor-smoke
echo $? # Expected: 42
```

This is a compiler smoke test for you to run against your current checkout. The extension itself only highlights text; it cannot validate compilation.
