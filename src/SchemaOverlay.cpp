#include <cursespp/SchemaOverlay.h>

#include <f8n/environment/Environment.h>
#include <f8n/sdk/ISchema.h>
#include <f8n/i18n/Locale.h>
#include <f8n/utf/conv.h>

#include <cursespp/App.h>
#include <cursespp/Colors.h>
#include <cursespp/DialogOverlay.h>
#include <cursespp/InputOverlay.h>
#include <cursespp/ListOverlay.h>
#include <cursespp/Screen.h>
#include <cursespp/ScrollAdapterBase.h>
#include <cursespp/SingleLineEntry.h>
#include <cursespp/Text.h>

using namespace f8n::sdk;
using namespace f8n::prefs;
using namespace f8n::i18n;
using namespace f8n::utf;
using namespace f8n::env;
using namespace cursespp;

using PrefsPtr = std::shared_ptr<Preferences>;
using SinglePtr = std::shared_ptr<SingleLineEntry>;
using SchemaPtr = std::shared_ptr<ISchema>;

#define DEFAULT(type) reinterpret_cast<const ISchema::type*>(entry)->defaultValue

static size_t DEFAULT_INPUT_WIDTH = 26;
static size_t MINIMUM_OVERLAY_WIDTH = 16;

static int overlayWidth() {
    return (int)(0.8f * (float) Screen::GetWidth());
}

static std::string stringValueForDouble(const double value, const int precision = 2) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(precision) << value;
    return out.str();
}

static std::function<std::string(int)> INT_FORMATTER =
[](int value) -> std::string {
    return std::to_string(value);
};

static std::function<std::string(double)> doubleFormatter(int precision) {
    return [precision](double value) -> std::string {
        return stringValueForDouble(value, precision);
    };
}

template <typename T>
bool bounded(T minimum, T maximum) {
    return
        minimum != std::numeric_limits<T>::min() &&
        maximum != std::numeric_limits<T>::max();
}

template <typename T>
std::string numberInputTitle(
    std::string keyName,
    T minimum,
    T maximum,
    std::function<std::string(T)> formatter)
{
    if (bounded(minimum, maximum)) {
        return keyName + " (" + formatter(minimum)
            + " - " + formatter(maximum) + ")";
    }
    return keyName;
}

template <typename T>
static std::string stringValueFor(
    PrefsPtr prefs,
    const T* entry,
    ISchema::Type type,
    const std::string& name)
{
    switch (type) {
        case ISchema::Type::Bool:
            return prefs->GetBool(name, DEFAULT(BoolEntry)) ? "true" : "false";
        case ISchema::Type::Int:
            return std::to_string(prefs->GetInt(name, DEFAULT(IntEntry)));
        case ISchema::Type::Double: {
            auto doubleEntry = reinterpret_cast<const ISchema::DoubleEntry*>(entry);
            auto defaultValue = doubleEntry->defaultValue;
            auto precision = doubleEntry->precision;
            return stringValueForDouble(prefs->GetDouble(name, defaultValue), precision);
        }
        case ISchema::Type::String:
            return prefs->GetString(name, DEFAULT(StringEntry));
        case ISchema::Type::Enum:
            return prefs->GetString(name, DEFAULT(EnumEntry));
    }
    throw std::runtime_error("invalid type passed to stringValueFor!");
}

template <typename T>
static std::string stringValueFor(PrefsPtr prefs, const T* entry) {
    return stringValueFor(prefs, entry, entry->entry.type, entry->entry.name);
}

static std::string stringValueFor(PrefsPtr prefs, const ISchema::Entry* entry) {
    return stringValueFor(prefs, entry, entry->type, entry->name);
}

template <typename T>
struct NumberValidator : public InputOverlay::IValidator {
    using Formatter = std::function<std::string(T)>;

    NumberValidator(T minimum, T maximum, Formatter formatter)
        : minimum(minimum), maximum(maximum), formatter(formatter) {
    }

    virtual bool IsValid(const std::string& input) const override {
        try {
            double result = std::stod(input);
            if (bounded(minimum, maximum) && (result < minimum || result > maximum)) {
                return false;
            }
        }
        catch (std::invalid_argument) {
            return false;
        }
        return true;
    }

    virtual const std::string ErrorMessage() const override {
        if (bounded(minimum, maximum)) {
            std::string result = _TSTR("validator_dialog_number_parse_bounded_error");
            ReplaceAll(result, "{{minimum}}", formatter(minimum));
            ReplaceAll(result, "{{maximum}}", formatter(maximum));
            return result;
        }
        return _TSTR("validator_dialog_number_parse_error");
    }

    Formatter formatter;
    T minimum, maximum;
};

class StringListAdapter : public ScrollAdapterBase {
    public:
        StringListAdapter(std::vector<std::string>& items) : items(items) { }
        std::string At(const size_t index) { return items[index]; }
        virtual ~StringListAdapter() { }
        virtual size_t GetEntryCount() override { return items.size(); }
        virtual EntryPtr GetEntry(cursespp::ScrollableWindow* window, size_t index) override {
            auto entry = std::make_shared<SingleLineEntry>(
                text::Ellipsize(items[index], window->GetWidth()));

            entry->SetAttrs(Color(Color::Default));
            if (index == window->GetScrollPosition().logicalIndex) {
                entry->SetAttrs(Color(Color::ListItemHighlighted));
            }

            return entry;
        }
    private:
        std::vector<std::string> items;
};

class SchemaAdapter: public ScrollAdapterBase {
    public:
        SchemaAdapter(PrefsPtr prefs, SchemaPtr schema): prefs(prefs), schema(schema) {
            onChanged = [this](std::string value) {
                this->changed = true;
            };
        }

        virtual ~SchemaAdapter() {
        }

        bool Changed() const {
            return this->changed;
        }

        virtual size_t GetEntryCount() override {
            return schema->Count();
        }

        virtual EntryPtr GetEntry(cursespp::ScrollableWindow* window, size_t index) override {
            auto entry = schema->At(index);

            std::string name = entry->name;
            std::string value = stringValueFor(prefs, entry);
            int width = window->GetContentWidth();
            int avail = std::max(0, width - int(u8cols(name)) - 1 - 1);
            auto display = " " + name + " " + text::Align(value + " ", text::AlignRight, avail);

            SinglePtr result = SinglePtr(new SingleLineEntry(text::Ellipsize(display, width)));

            result->SetAttrs(Color(Color::Default));
            if (index == window->GetScrollPosition().logicalIndex) {
                result->SetAttrs(Color(Color::ListItemHighlighted));
            }

            return result;
        }

        void ShowOverlay(size_t index) {
            auto entry = schema->At(index);
            switch (entry->type) {
                case ISchema::Type::Bool:
                    return ShowBoolOverlay(reinterpret_cast<const ISchema::BoolEntry*>(entry));
                case ISchema::Type::Int:
                    return ShowIntOverlay(reinterpret_cast<const ISchema::IntEntry*>(entry));
                case ISchema::Type::Double:
                    return ShowDoubleOverlay(reinterpret_cast<const ISchema::DoubleEntry*>(entry));
                case ISchema::Type::String:
                    return ShowStringOverlay(reinterpret_cast<const ISchema::StringEntry*>(entry));
                case ISchema::Type::Enum:
                    return ShowEnumOverlay(reinterpret_cast<const ISchema::EnumEntry*>(entry));
            }
        }

    private:
        void ShowBoolOverlay(const ISchema::BoolEntry* entry) {
            SchemaOverlay::ShowBoolOverlay(entry, prefs, onChanged);
        }

        void ShowIntOverlay(const ISchema::IntEntry* entry) {
            SchemaOverlay::ShowIntOverlay(entry, prefs, onChanged);
        }

        void ShowDoubleOverlay(const ISchema::DoubleEntry* entry) {
            SchemaOverlay::ShowDoubleOverlay(entry, prefs, onChanged);
        }

        void ShowStringOverlay(const ISchema::StringEntry* entry) {
            SchemaOverlay::ShowStringOverlay(entry, prefs, onChanged);
        }

        void ShowEnumOverlay(const ISchema::EnumEntry* entry) {
            SchemaOverlay::ShowEnumOverlay(entry, prefs, onChanged);
        }

        std::function<void(std::string)> onChanged;
        PrefsPtr prefs;
        SchemaPtr schema;
        bool changed{false};
};

void SchemaOverlay::ShowListOverlay(
    const std::string& title,
    std::vector<std::string>& items,
    const std::string defaultValue,
    std::function<void(std::string)> cb)
{
    auto stringAdapter = std::make_shared<StringListAdapter>(items);
    std::shared_ptr<ListOverlay> dialog(new ListOverlay());

    size_t index = 0;
    for (size_t i = 0; i < items.size(); i++) {
        auto current = items[i];
        if (current == defaultValue) {
            index = i;
        }
    }

    dialog->SetAdapter(stringAdapter)
        .SetTitle(title)
        .SetWidth(overlayWidth())
        .SetSelectedIndex(index)
        .SetAutoDismiss(true)
        .SetItemSelectedCallback(
            [cb, stringAdapter](ListOverlay* overlay, IScrollAdapterPtr adapter, size_t index) {
                if (cb) {
                    cb(stringAdapter->At(index));
                }
            });

    cursespp::App::Overlays().Push(dialog);
}

void SchemaOverlay::ShowBoolOverlay(
    const ISchema::BoolEntry* entry,
    PrefsPtr prefs,
    std::function<void(std::string)> callback)
{
    std::string name(entry->entry.name);
    std::vector<std::string> items = { "true", "false" };

    auto handler = [prefs, name, callback](std::string value) {
        prefs->SetBool(name, value == "true");
        if (callback) { callback(value); }
    };

    ShowListOverlay(name, items, stringValueFor(prefs, entry), handler);
}

void SchemaOverlay::ShowIntOverlay(
    const ISchema::IntEntry* entry,
    PrefsPtr prefs,
    std::function<void(std::string)> callback)
{
    std::string name(entry->entry.name);

    auto title = numberInputTitle(
        name, entry->minValue, entry->maxValue, INT_FORMATTER);

    auto validator = std::make_shared<NumberValidator<int>>(
        entry->minValue,  entry->maxValue, INT_FORMATTER);

    auto handler = [prefs, name, callback](std::string value) {
        prefs->SetInt(name, (int) std::stod(value));
        if (callback) { callback(value); }
    };

    std::shared_ptr<InputOverlay> dialog(new InputOverlay());

    dialog->SetTitle(title)
        .SetText(stringValueFor(prefs, entry))
        .SetValidator(validator)
        .SetWidth(overlayWidth())
        .SetInputAcceptedCallback(callback);

    App::Overlays().Push(dialog);
}

void SchemaOverlay::ShowDoubleOverlay(
    const ISchema::DoubleEntry* entry,
    PrefsPtr prefs,
    std::function<void(std::string)> callback)
{
    std::string name(entry->entry.name);

    auto formatter = doubleFormatter(entry->precision);

    auto title = numberInputTitle(
        name, entry->minValue, entry->maxValue, formatter);

    auto validator = std::make_shared<NumberValidator<double>>(
        entry->minValue, entry->maxValue, formatter);

    auto handler = [prefs, name, callback](std::string value) {
        prefs->SetDouble(name, std::stod(value));
        if (callback) { callback(value); }
    };

    std::shared_ptr<InputOverlay> dialog(new InputOverlay());

    dialog->SetTitle(title)
        .SetText(stringValueFor(prefs, entry))
        .SetValidator(validator)
        .SetWidth(overlayWidth())
        .SetInputAcceptedCallback(handler);

    App::Overlays().Push(dialog);
}

void SchemaOverlay::ShowStringOverlay(
    const ISchema::StringEntry* entry,
    PrefsPtr prefs,
    std::function<void(std::string)> callback)
{
    std::string name(entry->entry.name);

    auto handler = [prefs, name, callback](std::string value) {
        prefs->SetString(name, value.c_str());
        if (callback) { callback(value); }
    };

    std::shared_ptr<InputOverlay> dialog(new InputOverlay());

    dialog->SetTitle(name)
        .SetText(stringValueFor(prefs, entry))
        .SetWidth(overlayWidth())
        .SetInputAcceptedCallback(handler);

    App::Overlays().Push(dialog);
}

void SchemaOverlay::ShowEnumOverlay(
    const ISchema::EnumEntry* entry,
    PrefsPtr prefs,
    std::function<void(std::string)> callback)
{
    std::string name(entry->entry.name);
    std::vector<std::string> items;

    for (size_t i = 0; i < entry->count; i++) {
        items.push_back(entry->values[i]);
    }

    auto handler = [prefs, name, callback](std::string value) {
        prefs->SetString(name, value.c_str());
        if (callback) { callback(value); }
    };

    ShowListOverlay(name, items, stringValueFor(prefs, entry), handler);
}

void SchemaOverlay::Show(
    const std::string& title,
    PrefsPtr prefs,
    SchemaPtr schema,
    std::function<void(bool)> callback)
{
    std::shared_ptr<SchemaAdapter> schemaAdapter(new SchemaAdapter(prefs, schema));
    std::shared_ptr<ListOverlay> dialog(new ListOverlay());

    dialog->SetAdapter(schemaAdapter)
        .SetTitle(title)
        .SetWidthPercent(80)
        .SetAutoDismiss(false)
        .SetItemSelectedCallback(
            [schemaAdapter](ListOverlay* overlay, IScrollAdapterPtr adapter, size_t index) {
                schemaAdapter->ShowOverlay(index);
            })
        .SetDismissedCallback([callback, schemaAdapter](ListOverlay* overlay) {
                if (callback) {
                    callback(schemaAdapter->Changed());
                }
            });

    cursespp::App::Overlays().Push(dialog);
}