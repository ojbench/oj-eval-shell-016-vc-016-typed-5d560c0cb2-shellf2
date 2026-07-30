#include <bits/stdc++.h>
using namespace std;

struct FastInput {
    static constexpr size_t BUFSIZE = 1 << 20;
    char buf[BUFSIZE];
    size_t idx = 0, size = 0;

    inline char read() {
        if (idx >= size) {
            size = fread(buf, 1, BUFSIZE, stdin);
            idx = 0;
            if (size == 0) return 0;
        }
        return buf[idx++];
    }

    bool readInt(int &out) {
        char c;
        do {
            c = read();
            if (!c) return false;
        } while (c <= ' ');
        int sign = 1;
        if (c == '-') {
            sign = -1;
            c = read();
        }
        int val = 0;
        while (c > ' ') {
            val = val * 10 + (c - '0');
            c = read();
        }
        out = val * sign;
        return true;
    }

    bool readToken(char *s) {
        char c;
        do {
            c = read();
            if (!c) return false;
        } while (c <= ' ');
        int p = 0;
        while (c > ' ') {
            s[p++] = c;
            c = read();
        }
        s[p] = '\0';
        return true;
    }
};

struct FastOutput {
    static constexpr size_t BUFSIZE = 1 << 20;
    char buf[BUFSIZE];
    size_t idx = 0;

    ~FastOutput() { flush(); }

    inline void push(char c) {
        if (idx == BUFSIZE) flush();
        buf[idx++] = c;
    }

    inline void writeInt(int x) {
        if (x == 0) {
            push('0');
            return;
        }
        if (x < 0) {
            push('-');
            x = -x;
        }
        char s[16];
        int n = 0;
        while (x > 0) {
            s[n++] = char('0' + x % 10);
            x /= 10;
        }
        while (n--) push(s[n]);
    }

    void flush() {
        if (idx) fwrite(buf, 1, idx, stdout);
        idx = 0;
    }
};

static constexpr uint64_t HASH_EMPTY = 0ULL;
static constexpr int NIL = 0;
static constexpr int MAXN = 300000 + 5;
static constexpr int TABLE_SIZE = 1 << 20;
static constexpr int TABLE_MASK = TABLE_SIZE - 1;
static constexpr const char *STATE_FILE = "storage.dat";
static constexpr const char *STATE_TMP = "storage.dat.tmp";

struct Node {
    int val;
    uint32_t pri;
    int l, r;
};

struct Database {
    vector<Node> nodes;
    vector<array<char, 65>> keys;
    vector<int> roots;
    vector<uint64_t> hashes;
    vector<int> ids;
    vector<uint8_t> used;
    uint32_t rng = 2463534242u;

    Database() {
        nodes.reserve(MAXN + 5);
        nodes.push_back({}); // index 0 is null
        keys.reserve(MAXN + 5);
        roots.reserve(MAXN + 5);
        hashes.assign(TABLE_SIZE, 0);
        ids.assign(TABLE_SIZE, -1);
        used.assign(TABLE_SIZE, 0);
    }

    static uint64_t hash_key(const char *s) {
        uint64_t h = 1469598103934665603ULL;
        while (*s) {
            h ^= (unsigned char)*s++;
            h *= 1099511628211ULL;
        }
        return h ? h : 1ULL;
    }

    static bool key_eq(const array<char, 65> &a, const char *b) {
        return strcmp(a.data(), b) == 0;
    }

    inline uint32_t next_rand() {
        uint32_t x = rng;
        x ^= x << 13;
        x ^= x >> 17;
        x ^= x << 5;
        rng = x;
        return x;
    }

    int new_node(int val) {
        nodes.push_back({val, next_rand(), NIL, NIL});
        return (int)nodes.size() - 1;
    }

    int find_slot(const char *s, uint64_t h) const {
        size_t pos = (size_t)h & TABLE_MASK;
        while (used[pos]) {
            int id = ids[pos];
            if (hashes[pos] == h && strcmp(keys[id].data(), s) == 0) return (int)pos;
            pos = (pos + 1) & TABLE_MASK;
        }
        return (int)pos;
    }

    int get_key_id(const char *s) const {
        uint64_t h = hash_key(s);
        size_t pos = (size_t)h & TABLE_MASK;
        while (used[pos]) {
            int id = ids[pos];
            if (hashes[pos] == h && strcmp(keys[id].data(), s) == 0) return id;
            pos = (pos + 1) & TABLE_MASK;
        }
        return -1;
    }

    int get_or_create_key(const char *s) {
        uint64_t h = hash_key(s);
        size_t pos = (size_t)h & TABLE_MASK;
        while (used[pos]) {
            int id = ids[pos];
            if (hashes[pos] == h && strcmp(keys[id].data(), s) == 0) return id;
            pos = (pos + 1) & TABLE_MASK;
        }
        int id = (int)keys.size();
        array<char, 65> k{};
        strncpy(k.data(), s, 64);
        keys.push_back(k);
        roots.push_back(NIL);
        hashes[pos] = h;
        ids[pos] = id;
        used[pos] = 1;
        return id;
    }

    void rotate_left(int &t) {
        int r = nodes[t].r;
        nodes[t].r = nodes[r].l;
        nodes[r].l = t;
        t = r;
    }

    void rotate_right(int &t) {
        int l = nodes[t].l;
        nodes[t].l = nodes[l].r;
        nodes[l].r = t;
        t = l;
    }

    void insert(int &t, int val) {
        if (t == NIL) {
            t = new_node(val);
            return;
        }
        if (val == nodes[t].val) return;
        if (val < nodes[t].val) {
            insert(nodes[t].l, val);
            if (nodes[nodes[t].l].pri < nodes[t].pri) rotate_right(t);
        } else {
            insert(nodes[t].r, val);
            if (nodes[nodes[t].r].pri < nodes[t].pri) rotate_left(t);
        }
    }

    void erase(int &t, int val) {
        if (t == NIL) return;
        if (val < nodes[t].val) {
            erase(nodes[t].l, val);
        } else if (val > nodes[t].val) {
            erase(nodes[t].r, val);
        } else {
            if (nodes[t].l == NIL) {
                t = nodes[t].r;
            } else if (nodes[t].r == NIL) {
                t = nodes[t].l;
            } else if (nodes[nodes[t].l].pri < nodes[nodes[t].r].pri) {
                rotate_right(t);
                erase(nodes[t].r, val);
            } else {
                rotate_left(t);
                erase(nodes[t].l, val);
            }
        }
    }

    void inorder(int t, FastOutput &out, bool &first) const {
        if (t == NIL) return;
        inorder(nodes[t].l, out, first);
        if (!first) out.push(' ');
        out.writeInt(nodes[t].val);
        first = false;
        inorder(nodes[t].r, out, first);
    }

    void load() {
        ifstream fin(STATE_FILE, ios::binary);
        if (!fin.good()) return;
        int keyCount = 0;
        fin.read(reinterpret_cast<char *>(&keyCount), sizeof(keyCount));
        if (!fin) return;
        for (int i = 0; i < keyCount; ++i) {
            unsigned char len = 0;
            fin.read(reinterpret_cast<char *>(&len), 1);
            array<char, 65> key{};
            fin.read(key.data(), len);
            key[len] = '\0';
            int cnt = 0;
            fin.read(reinterpret_cast<char *>(&cnt), sizeof(cnt));
            if (!fin) break;
            int id = get_or_create_key(key.data());
            for (int j = 0; j < cnt; ++j) {
                int v = 0;
                fin.read(reinterpret_cast<char *>(&v), sizeof(v));
                insert(roots[id], v);
            }
        }
    }

    void save() {
        ofstream fout(STATE_TMP, ios::binary | ios::trunc);
        int keyCount = 0;
        for (int i = 0; i < (int)roots.size(); ++i) {
            if (roots[i] != NIL) ++keyCount;
        }
        fout.write(reinterpret_cast<const char *>(&keyCount), sizeof(keyCount));
        for (int i = 0; i < (int)roots.size(); ++i) {
            if (roots[i] == NIL) continue;
            unsigned char len = (unsigned char)strlen(keys[i].data());
            fout.write(reinterpret_cast<const char *>(&len), 1);
            fout.write(keys[i].data(), len);
            vector<int> vals;
            vals.reserve(16);
            collect(roots[i], vals);
            int cnt = (int)vals.size();
            fout.write(reinterpret_cast<const char *>(&cnt), sizeof(cnt));
            for (int v : vals) fout.write(reinterpret_cast<const char *>(&v), sizeof(v));
        }
        fout.close();
        remove(STATE_FILE);
        rename(STATE_TMP, STATE_FILE);
    }

    void collect(int t, vector<int> &vals) const {
        if (t == NIL) return;
        collect(nodes[t].l, vals);
        vals.push_back(nodes[t].val);
        collect(nodes[t].r, vals);
    }
};

int main() {
    FastInput in;
    FastOutput out;
    Database db;
    db.load();

    int n;
    if (!in.readInt(n)) return 0;
    char cmd[16];
    char key[65];
    int val;

    for (int i = 0; i < n; ++i) {
        in.readToken(cmd);
        if (cmd[0] == 'i') {
            in.readToken(key);
            in.readInt(val);
            int id = db.get_or_create_key(key);
            db.insert(db.roots[id], val);
        } else if (cmd[0] == 'd') {
            in.readToken(key);
            in.readInt(val);
            int id = db.get_key_id(key);
            if (id != -1) db.erase(db.roots[id], val);
        } else {
            in.readToken(key);
            int id = db.get_key_id(key);
            if (id == -1 || db.roots[id] == NIL) {
                out.push('n'); out.push('u'); out.push('l'); out.push('l'); out.push('\n');
            } else {
                bool first = true;
                db.inorder(db.roots[id], out, first);
                out.push('\n');
            }
        }
    }

    db.save();
    out.flush();
    return 0;
}
