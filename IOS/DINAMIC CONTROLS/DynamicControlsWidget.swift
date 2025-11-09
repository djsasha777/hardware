import WidgetKit
import SwiftUI
import AppIntents

@available(iOSApplicationExtension 18.0, *)
struct SendPostIntent: AppIntent {
    static var title: LocalizedStringResource = "Send HTTP POST"

    @Parameter(title: "URL")
    var urlString: String

    @Parameter(title: "Login", default: "")
    var login: String

    @Parameter(title: "Password", default: "")
    var password: String

    func perform() async throws -> some IntentResult {
        guard let url = URL(string: urlString) else {
            throw IntentError("Invalid URL")
        }
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        if !login.isEmpty && !password.isEmpty {
            let authString = "\(login):\(password)"
            if let authData = authString.data(using: .utf8) {
                request.setValue("Basic \(authData.base64EncodedString())", forHTTPHeaderField: "Authorization")
            }
        }
        URLSession.shared.dataTask(with: request).resume()
        return .result()
    }
}

@available(iOSApplicationExtension 18.0, *)
struct ControlProvider: TimelineProvider {
    typealias Entry = SimpleEntry

    func placeholder(in context: Context) -> SimpleEntry {
        SimpleEntry(date: Date(), buttons: [])
    }

    func getSnapshot(in context: Context, completion: @escaping (SimpleEntry) -> ()) {
        let entry = SimpleEntry(date: Date(), buttons: [])
        completion(entry)
    }

    func getTimeline(in context: Context, completion: @escaping (Timeline<SimpleEntry>) -> ()) {
        // Здесь надо получить состояние кнопок из общего хранилища (например, App Group UserDefaults)
        // Для примера создаем статичные данные

        var entries: [SimpleEntry] = []
        let currentDate = Date()
        for offset in 0..<5 {
            let entryDate = Calendar.current.date(byAdding: .second, value: offset * 5, to: currentDate)!
            let buttons = [
                WidgetButton(name: "Button1", isOn: offset % 2 == 0, url: "https://example.com/api/btn1", login: "", password: ""),
                WidgetButton(name: "Button2", isOn: offset % 2 != 0, url: "https://example.com/api/btn2", login: "", password: "")
            ]
            entries.append(SimpleEntry(date: entryDate, buttons: buttons))
        }
        let timeline = Timeline(entries: entries, policy: .atEnd)
        completion(timeline)
    }
}

@available(iOSApplicationExtension 18.0, *)
struct SimpleEntry: TimelineEntry {
    let date: Date
    let buttons: [WidgetButton]
}

@available(iOSApplicationExtension 18.0, *)
struct WidgetButton: Identifiable {
    var id = UUID()
    var name: String
    var isOn: Bool
    var url: String
    var login: String
    var password: String
}

@available(iOSApplicationExtension 18.0, *)
struct DynamicControlWidgetEntryView : View {
    var entry: ControlProvider.Entry

    var body: some View {
        VStack {
            ForEach(entry.buttons) { button in
                Button(button.name) {
                    Task {
                        _ = try? await SendPostIntent(urlString: button.url, login: button.login, password: button.password).perform()
                    }
                }
                .padding(8)
                .frame(maxWidth: .infinity)
                .background(button.isOn ? Color.green : Color.red)
                .foregroundColor(.white)
                .cornerRadius(8)
            }
        }
        .padding()
    }
}

@main
struct DynamicControlWidget: Widget {
    let kind: String = "com.example.dynamiccontrolwidget"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: ControlProvider()) { entry in
            DynamicControlWidgetEntryView(entry: entry)
        }
        .configurationDisplayName("Динамический контрол")
        .description("Виджет с состоянием кнопок")
        .supportedFamilies([.systemSmall, .systemMedium])
    }
}
