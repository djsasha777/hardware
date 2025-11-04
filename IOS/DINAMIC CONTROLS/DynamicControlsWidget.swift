import WidgetKit
import SwiftUI
import AppIntents

// AppIntent для выполнения действия HTTP POST
@available(iOSApplicationExtension 18.0, *)
struct SendPostIntent: AppIntent {
    static var title: LocalizedStringResource = "Send HTTP POST"
    
    @Parameter(title: "URL")
    var urlString: String
    
    func perform() async throws -> some IntentResult {
        guard let url = URL(string: urlString) else {
            throw IntentError("Invalid URL")
        }
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        URLSession.shared.dataTask(with: request).resume()
        return .result()
    }
}

// Провайдер для Control Widget, показывает динамическое состояние (можно расширить)
@available(iOSApplicationExtension 18.0, *)
struct ControlProvider: ControlValueProvider {
    typealias Value = Void
    
    var previewValue: Value { () }
    
    func currentValue() async throws -> Value {
        return ()
    }
}

// Control Widget динамически создаст кнопки по конфигу
@available(iOSApplicationExtension 18.0, *)
struct DynamicControlWidget: ControlWidget {
    let kind: String = "com.example.dynamiccontrolwidget"
    
    var body: some ControlWidgetConfiguration {
        StaticControlConfiguration(kind: kind, provider: ControlProvider()) { _ in
            VStack {
                // Пример статичной кнопки (для многокнопочного виджета сделайте механизм генерации)
                Button("Send Request") {
                    SendPostIntent(urlString: "https://spongo.ru/someendpoint").perform()
                }
                .controlWidgetAction(SendPostIntent(urlString: "https://spongo.ru/someendpoint"))
                .tint(.blue)
            }
        }
        .configurationDisplayName("Dynamic Controls")
        .description("Widget with dynamic HTTP POST buttons")
    }
}
