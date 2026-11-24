#include "dieselprijzen.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

namespace esphome::dieselprijzen {
using tessera::Font;
namespace ui = tessera::ui;

struct Station {
  std::string name;
  float price;
};

// The source sensor can expose stations/lijst/results/data as a list of objects.
// Reject unknown prices rather than displaying zero or a fabricated number.
static float number(JsonVariantConst v) {
  if (v.is<float>() || v.is<double>() || v.is<int>()) return v.as<float>();
  if (!v.is<const char *>()) return NAN;
  std::string s = v.as<std::string>();
  std::replace(s.begin(), s.end(), ',', '.');
  char *end = nullptr;
  float p = strtof(s.c_str(), &end);
  return end == s.c_str() || !std::isfinite(p) ? NAN : p;
}
static std::string name_of(JsonObjectConst o) {
  for (const char *key : {"name", "naam", "station_name", "title", "tankstation", "station"}) {
    if (o[key].is<const char *>()) {
      std::string name = o[key].as<std::string>();
      if (!name.empty()) {
        for (const char *place : {"city", "plaats", "place", "locality"}) {
          if (o[place].is<const char *>()) {
            std::string city = o[place].as<std::string>();
            if (!city.empty() && name.find(city) == std::string::npos) name += " - " + city;
            break;
          }
        }
        return name;
      }
    }
  }
  return "";
}
static float price_of(JsonObjectConst o) {
  for (const char *key : {"price", "prijs", "diesel", "diesel_price", "fuel_price", "value"}) {
    float p = number(o[key]);
    if (std::isfinite(p) && p > 0.5f && p < 5.0f) return p;
  }
  return NAN;
}

class PricesTile : public tessera::Tile {
 public:
  explicit PricesTile(const tessera::Plugin *plugin) : plugin_(plugin) {}
  void create(const tessera::TileContext &c) override {
    width_ = c.width;
    height_ = c.height;
    title_ = ui::label(c.parent, Font::TITLE, theme::INK);
    lv_obj_set_width(title_, width_);
    ui::set_text(title_, plugin_->text("heading"));
    // Create the labels only once; move and update them when sensor data changes.
    for (int i = 0; i < MAX_ROWS; ++i) {
      names_[i] = ui::label(c.parent, Font::BODY, theme::INK);
      values_[i] = ui::label(c.parent, Font::BODY, theme::INK);
      lv_obj_set_style_text_align(values_[i], LV_TEXT_ALIGN_RIGHT, 0);
    }
    notice_ = ui::label(c.parent, Font::BODY, theme::MUTED);
    draw();
  }
  void on_state(JsonObjectConst data) override {
    stations_.clear();
    stale_ = data["stale"] | false;
    if (data["items"].is<JsonArrayConst>()) {
      for (JsonVariantConst item : data["items"].as<JsonArrayConst>()) {
        if (!item.is<JsonObjectConst>()) continue;
        JsonObjectConst row = item.as<JsonObjectConst>();
        float lat = number(row["lat"]);
        float lon = number(row["lon"]);
        if (!std::isfinite(lat) || !std::isfinite(lon) || distance_km(lat, lon) > 15.0f)
          continue;
        float price = NAN;
        // The ANWB API lists several fuels per station. Inspect each mapped slot;
        // do NOT assume diesel is the first price.
        for (int i = 0; i < 4; ++i) {
          char ty[12], va[12];
          snprintf(ty, sizeof(ty), "type%d", i);
          snprintf(va, sizeof(va), "value%d", i);
          const char *kind = row[ty] | "";
          if (strcmp(kind, "DIESEL") != 0) continue;
          float candidate = number(row[va]);
          if (std::isfinite(candidate) && candidate > 0.5f && candidate < 5.0f)
            price = candidate;
          break;
        }
        if (!std::isfinite(price)) continue;
        std::string name = row["name"] | "Tankstation";
        std::string city = row["city"] | "";
        if (!city.empty() && name.find(city) == std::string::npos) name += " - " + city;
        stations_.push_back({name, price});
      }
    }
    std::sort(stations_.begin(), stations_.end(), [](const Station &a, const Station &b) {
      if (a.price != b.price) return a.price < b.price;
      return a.name < b.name;
    });
    draw();
  }
  void on_theme() override {
    ui::set_color(title_, theme::INK);
    ui::set_color(notice_, theme::MUTED);
    for (int i = 0; i < MAX_ROWS; ++i) {
      ui::set_color(names_[i], theme::INK);
      ui::set_color(values_[i], theme::INK);
    }
  }
 private:
  static constexpr int MAX_ROWS = 8;
  bool stale_{false};
  static float distance_km(float lat, float lon) {
    constexpr float pi = 3.14159265358979323846f;
    const float p1 = 53.0553f * pi / 180.0f;
    const float p2 = lat * pi / 180.0f;
    const float dp = (lat - 53.0553f) * pi / 180.0f;
    const float dl = (lon - 6.7273f) * pi / 180.0f;
    const float a = sinf(dp / 2) * sinf(dp / 2) + cosf(p1) * cosf(p2) * sinf(dl / 2) * sinf(dl / 2);
    return 6371.0088f * 2 * atan2f(sqrtf(a), sqrtf(std::max(0.0f, 1.0f - a)));
  }
  void draw() {
    if (!title_) return;
    const int pad = ui::px(6);
    const int head = ui::line_height(Font::TITLE) + pad;
    lv_obj_set_pos(title_, 0, 0);
    lv_obj_set_height(title_, ui::line_height(Font::TITLE));
    const int available = std::max(0, height_ - head);
    const int step = ui::line_height(Font::BODY) + ui::px(4);
    const int count = std::min(static_cast<int>(stations_.size()), std::min(MAX_ROWS, available / std::max(1, step)));
    const int price_width = ui::px(90);
    for (int i = 0; i < MAX_ROWS; ++i) {
      if (i >= count) {
        lv_obj_add_flag(names_[i], LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(values_[i], LV_OBJ_FLAG_HIDDEN);
        continue;
      }
      lv_obj_remove_flag(names_[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_remove_flag(values_[i], LV_OBJ_FLAG_HIDDEN);
      lv_obj_set_pos(names_[i], 0, head + i * step);
      lv_obj_set_size(names_[i], std::max(1, width_ - price_width - pad), ui::line_height(Font::BODY));
      lv_obj_set_pos(values_[i], std::max(0, width_ - price_width), head + i * step);
      lv_obj_set_size(values_[i], price_width, ui::line_height(Font::BODY));
      ui::set_text(names_[i], stations_[i].name);
      char buffer[24];
      snprintf(buffer, sizeof(buffer), "%.3f", stations_[i].price);
      ui::set_text(values_[i], buffer);
    }
    lv_obj_set_pos(notice_, 0, head);
    lv_obj_set_size(notice_, width_, ui::line_height(Font::BODY));
    ui::set_text(notice_, count ? (stale_ ? plugin_->text("stale") : "") : plugin_->text("no_prices"));
  }
  const tessera::Plugin *plugin_;
  int width_{0}, height_{0};
  lv_obj_t *title_{}, *notice_{}, *names_[MAX_ROWS]{}, *values_[MAX_ROWS]{};
  std::vector<Station> stations_;
};

void Dieselprijzen::setup() {
  add_tile("prices", [this]() { return new PricesTile(this); });
}
}  // namespace esphome::dieselprijzen
