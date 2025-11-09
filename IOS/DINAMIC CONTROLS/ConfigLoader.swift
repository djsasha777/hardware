import Foundation
import Combine

class ConfigLoader: ObservableObject {
    @Published var buttons: [ButtonConfig] = []
    @Published var errorMessage: String? = nil
    private var timer: Timer?

    func startStatusPolling() {
        timer?.invalidate()
        timer = Timer.scheduledTimer(withTimeInterval: 5.0, repeats: true, block: { [weak self] _ in
            self?.updateButtonStatuses()
        })
    }

    func updateButtonStatuses() {
        for i in buttons.indices {
            let button = buttons[i]
            guard let statusUrl = URL(string: button.url + "/status") else { continue }
            URLSession.shared.dataTask(with: statusUrl) { [weak self] data, _, error in
                DispatchQueue.main.async {
                    if let error = error {
                        self?.errorMessage = "Ошибка сети: \(error.localizedDescription)"
                        return
                    }
                    guard let data = data,
                        let statusString = String(data: data, encoding: .utf8)?
                            .trimmingCharacters(in: .whitespacesAndNewlines).lowercased()
                    else {
                        self?.errorMessage = "Некорректные данные состояния"
                        return
                    }
                    self?.buttons[i].isOn = (statusString == "on")
                }
            }.resume()
        }
    }

    func sendPostRequest(to button: ButtonConfig) {
        guard let url = URL(string: button.url) else { return }
        var request = URLRequest(url: url)
        request.httpMethod = "POST"
        if button.secure, let login = button.login, let password = button.password {
            let authString = "\(login):\(password)"
            if let authData = authString.data(using: .utf8) {
                request.setValue("Basic \(authData.base64EncodedString())", forHTTPHeaderField: "Authorization")
            }
        }
        URLSession.shared.dataTask(with: request).resume()
    }

    func addButton(_ button: ButtonConfig) {
        buttons.append(button)
    }

    func addButtons(_ newButtons: [ButtonConfig]) {
        buttons.append(contentsOf: newButtons)
    }

    func updateButton(_ updatedButton: ButtonConfig) {
        if let index = buttons.firstIndex(where: { $0.id == updatedButton.id }) {
            buttons[index] = updatedButton
        }
    }

    func deleteButton(_ button: ButtonConfig) {
        buttons.removeAll { $0.id == button.id }
    }
}

struct ButtonConfig: Identifiable, Codable, Equatable {
    let id: UUID
    var name: String
    var url: String
    var type: String
    var conditions: [String]
    var secure: Bool
    var login: String?
    var password: String?
    var isOn: Bool = false
}
