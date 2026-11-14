# `std.crypto`

The current `stdlib/crypto.npp` module exposes `hash_sha256(text: string) -> string`. It wraps a built-in SHA-256 implementation and returns the digest as a 64-character lowercase hexadecimal string.

## Example

```npp
import std.crypto

fn main() {
    let digest = hash_sha256("N++")
    print(digest)
}
```

The current wrapper hashes the UTF-8/string bytes. It does not expose streaming hashing, HMAC, encryption, signature verification, cryptographically secure random numbers or password hashing.

**Security note:** SHA-256 is a general-purpose digest, not a password-storage algorithm. Do not store passwords as a plain SHA-256 digest; use a dedicated password-hashing algorithm such as Argon2id or scrypt in a suitable maintained library.
