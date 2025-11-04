import WidgetKit
import SwiftUI

struct ButtonConfigWidget: Identifiable, Codable {
    var id = UUID()
    let name: String
    let url: String
}

struct Provider: TimelineProvider {
    typealias Entry = SimpleEntry

    func placeholder(in context: Context) -> SimpleEntry {
        SimpleEntry(date: Date(), button: ButtonConfigWidget(id: UUID(), name: "Loading...", url: ""))
    }

    func getSnapshot(in context: Context, completion: @escaping (SimpleEntry) -> ()) {
        completion(placeholder(in: context))
    }

    func getTimeline(in context: Context, completion: @escaping (Timeline<SimpleEntry>) -> ()) {
        // For demo, static button. Real app should pass from shared storage or update dynamically
        let entry = SimpleEntry(date: Date(), button: ButtonConfigWidget(id: UUID(), name: "Placeholder", url: ""))
        let timeline = Timeline(entries: [entry], policy: .never)
        completion(timeline)
    }
}

struct SimpleEntry: TimelineEntry {
    let date: Date
    let button: ButtonConfigWidget
}

struct DynamicControlWidgetEntryView : View {
    var entry: Provider.Entry
    
    var body: some View {
        Button(action: {
            sendPost(urlString: entry.button.url)
        }) {
            Text(entry.button.name)
                .frame(maxWidth: .infinity, maxHeight: .infinity)
        }
        .buttonStyle(.borderedProminent)
    }
}

@main
struct DynamicControlsWidget: Widget {
    let kind: String = "DynamicControlsWidget"

    var body: some WidgetConfiguration {
        StaticConfiguration(kind: kind, provider: Provider()) { entry in
            DynamicControlWidgetEntryView(entry: entry)
        }
        .configurationDisplayName("Control Center Button")
        .description("Dynamic button for Control Center")
        .supportedFamilies([.systemSmall]) // адаптация под Control Center
    }
}

// Простая функция послать POST-запрос
func sendPost(urlString: String) {
    guard let url = URL(string: urlString) else { return }
    var request = URLRequest(url: url)
    request.httpMethod = "POST"
    URLSession.shared.dataTask(with: request).resume()
}
