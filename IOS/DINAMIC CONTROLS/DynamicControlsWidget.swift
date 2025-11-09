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
                let authValue = "Basic \(authData.base64EncodedString())"
                request.setValue(authValue, forHTTPHeaderField: "Authorization")
            }
        }
        URLSession.shared.dataTask(with: request).resume()
        return .result()
    }
}

@available(iOSApplicationExtension 18.0, *)
struct ControlProvider: ControlValueProvider {
    typealias Value = Void
    var previewValue: Value { () }
    func currentValue() async throws -> Value { () }
}

@available(iOSApplicationExtension 18.0, *)
struct DynamicControlWidget: ControlWidget {
    let kind: String = "com.example.dynamiccontrolwidget"

    var body: some ControlWidgetConfiguration {
        StaticControlConfiguration(kind: kind, provider: ControlProvider()) { _ in
            VStack {
                // Здесь можно динамически загружать кнопки из общего хранилища конфигурации

                // Пример кнопки
                Button("Send Request") {
                    _ = try? await SendPostIntent(urlString: "https://spongo.ru/someendpoint", login: "", password: "").perform()
                }
                .tint(.blue)
            }
        }
        .configurationDisplayName("Dynamic Controls")
        .description("Widget with dynamic HTTP POST buttons")
    }
}
