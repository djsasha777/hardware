import SwiftUI

@main
struct DynamicControlCenterApp: App {
    @StateObject var configLoader = ConfigLoader()

    var body: some Scene {
        WindowGroup {
            ContentView()
                .environmentObject(configLoader)
        }
    }
}

struct ContentView: View {
    @EnvironmentObject var configLoader: ConfigLoader
    @State private var showingAddButton = false
    @State private var showingImport = false
    @State private var editingButton: ButtonConfig?
    @State private var showAlert = false

    var body: some View {
        VStack {
            HStack {
                Button(action: {
                    editingButton = nil
                    showingAddButton = true
                }) {
                    Label("Плюс", systemImage: "plus")
                        .font(.title2)
                }
                .padding()
                Spacer()
                Button(action: { showingImport = true }) {
                    Label("Импорт", systemImage: "square.and.arrow.down")
                        .font(.title2)
                }
                .padding()
            }
            .padding(.horizontal)

            ScrollView {
                LazyVStack(spacing: 12) {
                    ForEach($configLoader.buttons) { $button in
                        Button(action: {
                            withAnimation(.easeInOut(duration: 0.3)) {
                                configLoader.sendPostRequest(to: button)
                            }
                        }) {
                            Text(button.name)
                                .padding()
                                .frame(maxWidth: .infinity)
                                .background(button.isOn ? Color.blue : Color.gray)
                                .scaleEffect(button.isOn ? 1.05 : 1.0)
                                .foregroundColor(.white)
                                .cornerRadius(10)
                                .animation(.easeInOut(duration: 0.4), value: button.isOn)
                        }
                        .contextMenu {
                            Button("Изменить") {
                                editingButton = button
                                showingAddButton = true
                            }
                            Button("Удалить", role: .destructive) {
                                configLoader.deleteButton(button)
                            }
                        }
                    }
                }
                .padding()
            }
        }
        .sheet(isPresented: $showingAddButton) {
            AddButtonView(configLoader: configLoader, buttonToEdit: $editingButton)
        }
        .sheet(isPresented: $showingImport) {
            ImportView(configLoader: configLoader)
        }
        .onAppear {
            configLoader.startStatusPolling()
        }
        .alert("Ошибка", isPresented: $showAlert, actions: {
            Button("OK", role: .cancel) {}
        }, message: {
            Text(configLoader.errorMessage ?? "Неизвестная ошибка")
        })
        .onReceive(configLoader.$errorMessage) { errorMsg in
            if errorMsg != nil { showAlert = true }
        }
    }
}

struct AddButtonView: View {
    @Environment(\.dismiss) var dismiss
    @ObservedObject var configLoader: ConfigLoader
    @Binding var buttonToEdit: ButtonConfig?

    @State private var name = ""
    @State private var url = ""
    @State private var secure = false
    @State private var login = ""
    @State private var password = ""
    let conditions = ["on"]

    var body: some View {
        NavigationView {
            Form {
                TextField("Имя кнопки", text: $name)
                TextField("URL", text: $url)
                    .autocapitalization(.none)
                    .disableAutocorrection(true)
                Toggle("Secure", isOn: $secure)
                if secure {
                    TextField("Логин", text: $login)
                        .autocapitalization(.none)
                        .disableAutocorrection(true)
                    SecureField("Пароль", text: $password)
                }
            }
            .navigationTitle(buttonToEdit == nil ? "Добавить кнопку" : "Редактировать кнопку")
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("Сохранить") {
                        let newButton = ButtonConfig(
                            id: buttonToEdit?.id ?? UUID(),
                            name: name,
                            url: url,
                            type: "button",
                            conditions: conditions,
                            secure: secure,
                            login: secure ? login : nil,
                            password: secure ? password : nil)
                        if buttonToEdit != nil {
                            configLoader.updateButton(newButton)
                        } else {
                            configLoader.addButton(newButton)
                        }
                        dismiss()
                    }
                    .disabled(name.isEmpty || url.isEmpty || (secure && (login.isEmpty || password.isEmpty)))
                }
                ToolbarItem(placement: .cancellationAction) {
                    Button("Отмена", role: .cancel) { dismiss() }
                }
            }
            .onAppear {
                if let btn = buttonToEdit {
                    name = btn.name
                    url = btn.url
                    secure = btn.secure
                    login = btn.login ?? ""
                    password = btn.password ?? ""
                }
            }
        }
    }
}

struct ImportView: View {
    @Environment(\.dismiss) var dismiss
    @ObservedObject var configLoader: ConfigLoader
    @State private var importUrl = ""
    @State private var isLoading = false
    @State private var errorMessage: String?

    var body: some View {
        NavigationView {
            Form {
                Section {
                    TextField("URL JSON конфигурации", text: $importUrl)
                        .autocapitalization(.none)
                        .disableAutocorrection(true)
                    if let error = errorMessage {
                        Text(error).foregroundColor(.red)
                    }
                    if isLoading {
                        ProgressView()
                    }
                }
            }
            .navigationTitle("Импорт кнопок")
            .toolbar {
                ToolbarItem(placement: .confirmationAction) {
                    Button("Импорт") {
                        importButtons()
                    }
                    .disabled(importUrl.isEmpty || isLoading)
                }
                ToolbarItem(placement: .cancellationAction) {
                    Button("Отмена", role: .cancel) { dismiss() }
                }
            }
        }
    }

    func importButtons() {
        guard let url = URL(string: importUrl) else {
            errorMessage = "Некорректный URL"
            return
        }
        isLoading = true
        URLSession.shared.dataTask(with: url) { data, _, error in
            DispatchQueue.main.async {
                isLoading = false
                if let error = error {
                    errorMessage = "Ошибка загрузки: \(error.localizedDescription)"
                    return
                }
                guard let data = data else {
                    errorMessage = "Данные не получены"
                    return
                }
                do {
                    let importedButtons = try JSONDecoder().decode([ButtonConfig].self, from: data)
                    configLoader.addButtons(importedButtons)
                    dismiss()
                } catch {
                    errorMessage = "Ошибка разбора JSON: \(error.localizedDescription)"
                }
            }
        }.resume()
    }
}
