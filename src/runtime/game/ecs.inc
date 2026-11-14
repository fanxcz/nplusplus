int64_t npp_ecs_create() {
    int64_t id = gNextEntity++;
    gEntities[id] = EcsEntity{};
    return id;
}
void npp_ecs_destroy(int64_t id) {
    gEntities.erase(id);
}
bool npp_ecs_alive(int64_t id) {
    return gEntities.contains(id);
}
void npp_ecs_set_position(int64_t id, double x, double y) {
    auto it = gEntities.find(id);
    if (it != gEntities.end()) {
        it->second.x = float(x);
        it->second.y = float(y);
    }
}
void npp_ecs_set_velocity(int64_t id, double x, double y) {
    auto it = gEntities.find(id);
    if (it != gEntities.end()) {
        it->second.vx = float(x);
        it->second.vy = float(y);
    }
}
void npp_ecs_set_size(int64_t id, double w, double h) {
    auto it = gEntities.find(id);
    if (it != gEntities.end()) {
        it->second.w = float(w);
        it->second.h = float(h);
    }
}
void npp_ecs_set_color(int64_t id, double r, double g, double b, double a) {
    auto it = gEntities.find(id);
    if (it != gEntities.end()) {
        it->second.r = float(r);
        it->second.g = float(g);
        it->second.b = float(b);
        it->second.a = float(a);
    }
}
void npp_ecs_set_texture(int64_t id, int64_t tex) {
    auto it = gEntities.find(id);
    if (it != gEntities.end())
        it->second.texture = tex;
}
void npp_ecs_update(double dt) {
    for (auto& [id, e] : gEntities) {
        if (e.alive) {
            e.x += e.vx * float(dt);
            e.y += e.vy * float(dt);
        }
    }
}
void npp_ecs_draw() {
    if (!gGraphics)
        return;
    for (auto& [id, e] : gEntities) {
        if (!e.alive)
            continue;
        if (e.texture)
            npp_gfx_texture_draw(e.texture, e.x, e.y, e.w, e.h, e.r, e.g, e.b, e.a);
        else
            npp_gfx_rect(e.x, e.y, e.w, e.h, e.r, e.g, e.b, e.a);
    }
}
bool npp_ecs_collides(int64_t a, int64_t b) {
    auto ia = gEntities.find(a), ib = gEntities.find(b);
    if (ia == gEntities.end() || ib == gEntities.end())
        return false;
    const auto& A = ia->second;
    const auto& B = ib->second;
    return A.x < B.x + B.w && A.x + A.w > B.x && A.y < B.y + B.h && A.y + A.h > B.y;
}
double npp_ecs_x(int64_t id) {
    auto it = gEntities.find(id);
    return it == gEntities.end() ? 0 : it->second.x;
}
double npp_ecs_y(int64_t id) {
    auto it = gEntities.find(id);
    return it == gEntities.end() ? 0 : it->second.y;
}
int64_t npp_ecs_count() {
    return int64_t(gEntities.size());
}
