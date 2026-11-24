#include "dieselprijzen.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
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
    JsonVariantConst attrs = data["attributes"];
    JsonArrayConst stations;
    for (const char *key : {"stations", "lijst", "results", "data"}) {
      if (attrs[key].is<JsonArrayConst>()) { stations = attrs[key].as<JsonArrayConst>(); break; }
      if (attrs[key]["stations"].is<JsonArrayConst>()) {
        stations = attrs[key]["stations"].as<JsonArrayConst>(); break;
      }
    }
    for (JsonVariantConst item : stations) {
      if (!item.is<JsonObjectConst>()) continue;
      JsonObjectConst row = item.as<JsonObjectConst>();
      float price = price_of(row);
      std::string name = name_of(row);
      if (std::isfinite(price) && !name.empty()) stations_.push_back({name, price});
    }
    std::sort(stations_.begin(), stations_.end(), [](const Station &a, const Station &b) {
      return a.price < b.price;
    });
    // A single price sensor can also serve as a tile, with its friendly name.
    if (stations_.empty()) {
      float price = number(data["state"]);
      if (std::isfinite(price) && price > 0.5f && price < 5.0f) {
        std::string name = attrs["friendly_name"] | "Tankstation";
        stations_.push_back({name, price});
      }
    }
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
    ui::set_text(notice_, count ? "" : plugin_->text("no_prices"));
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
