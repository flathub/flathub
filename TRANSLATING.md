# Translating OpenKara

OpenKara uses [react-i18next](https://react.i18next.com/) with static JSON translation files. English (`en.json`) is the source of truth. Contributions for new languages are welcome.

## Adding a new language

1. **Copy the English file**

   ```
   cp src/locales/en.json src/locales/{code}.json
   ```

   Use a BCP 47 language code for the filename (see [Language codes](#language-codes) below).

2. **Translate all values**

   Open your new file and translate every JSON value. Do **not** change any keys.

3. **Register the language in `src/lib/i18n.ts`**

   - Import the new JSON file:

     ```ts
     import ja from '@/locales/ja.json';
     ```

   - Add it to the `resources` object in the `i18next.init()` call:

     ```ts
     resources: {
       en: { translation: en },
       'zh-CN': { translation: zhCN },
       ja: { translation: ja },       // <- add this
     },
     ```

   - Add an entry to `SUPPORTED_LANGUAGES`:

     ```ts
     export const SUPPORTED_LANGUAGES = [
       { code: 'en', name: 'English' },
       { code: 'zh-CN', name: '简体中文' },
       { code: 'ja', name: '日本語' },   // <- add this
     ] as const;
     ```

     Use the language's native name for `name`.

4. **Test**

   Run the app and switch to your language in **Settings > Language** to verify everything renders correctly.

## Translation guidelines

- **Keys are sacred.** Never rename, add, or remove keys. Only translate the values.
- **Keep `{{variable}}` placeholders as-is.** These are dynamic values injected at runtime (e.g., `"Separating {{current}}/{{total}}"`). Translate the surrounding text but leave the placeholders untouched.
- **Preserve special characters.** Keep unicode escapes like `\u2026` (ellipsis) or replace them with the actual character (`...`).
- **Don't translate brand names.** Keep "OpenKara", "LRC", and other product/format names unchanged.
- **Match the tone.** OpenKara's UI is concise and direct. Avoid overly formal or verbose translations.

## File structure

Translation files use a flat namespace of feature areas. Here's what each section covers:

| Namespace        | Description                                   |
| ---------------- | --------------------------------------------- |
| `common`         | Shared labels: Cancel, Save, Close, etc.      |
| `setup`          | First-run library setup wizard                |
| `sidebar`        | Sidebar navigation and batch separation       |
| `toolbar`        | Top toolbar actions                           |
| `library`        | Track list and context menu actions           |
| `player`         | Playback controls (play, pause, seek, volume) |
| `queue`          | Play queue panel                              |
| `stems`          | Stem mixer (vocals, drums, bass, etc.)        |
| `lyrics`         | Lyrics display and editing                    |
| `songEdit`       | Song metadata editing dialog                  |
| `songProperties` | Song properties/info dialog                   |
| `settings`       | Preferences panel                             |
| `bootstrap`      | AI model download and setup                   |
| `errors`         | Error messages and titles                     |

## Language codes

Use [BCP 47](https://www.rfc-editor.org/info/bcp47) language tags. Examples:

| Code    | Language               |
| ------- | ---------------------- |
| `en`    | English                |
| `zh-CN` | Simplified Chinese     |
| `ja`    | Japanese               |
| `ko`    | Korean                 |
| `fr`    | French                 |
| `de`    | German                 |
| `es`    | Spanish                |
| `pt-BR` | Brazilian Portuguese   |

Use the shortest code that uniquely identifies the language. Add a region subtag only when necessary to distinguish variants (e.g., `pt-BR` vs `pt-PT`).
