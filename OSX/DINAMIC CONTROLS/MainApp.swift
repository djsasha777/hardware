import SwiftUI

@main
struct DynamicControlCenterApp: App {
    @StateObject var configLoader = ConfigLoader()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(configLoader)
                .onAppear {
                    configLoader.loadConfig()
                }
        }
    }
}

struct ContentView: View {
    @EnvironmentObject var configLoader: ConfigLoader
    
    var body: some View {
        VStack(spacing: 20) {
            Text("Dynamic Controls").font(.title)
            ForEach(configLoader.buttons) { button in
                Button(button.name) {
                    sendPostRequest(urlString: button.url)
                }
                .frame(maxWidth: .infinity)
                .padding()
                .background(Color.blue.opacity(0.7))
                .foregroundColor(.white)
                .cornerRadius(10)
            }
        }
        .padding()
        .frame(minWidth: 300, minHeight: 400)
    }
}

func sendPostRequest(urlString: String) {
    guard let url = URL(string: urlString) else { return }
    var request = URLRequest(url: url)
    request.httpMethod = "POST"
    URLSession.shared.dataTask(with: request).resume()
}

class ConfigLoader: ObservableObject {
    @Published var buttons: [ButtonConfig] = []
    
    func loadConfig() {
        guard let url = URL(string: "https://spongo.ru/config.json") else { return }
        URLSession.shared.dataTask(with: url) { data, _, error in
            guard let data = data, error == nil else { return }
            do {
                let decoded = try JSONDecoder().decode([ButtonConfig].self, from: data)
                DispatchQueue.main.async {
                    self.buttons = decoded
                }
            } catch {
                print("JSON decode error: \(error)")
            }
        }.resume()
    }
}

struct ButtonConfig: Codable, Identifiable {
    var id = UUID()
    let name: String
    let url: String
}
