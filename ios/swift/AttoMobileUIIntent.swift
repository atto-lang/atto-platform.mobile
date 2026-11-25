// Transport model ONLY: all fields are plain values; no UIView, closure,
// native pointer or capability reference may cross an actor boundary.
public struct AttoMobileUIIntent: Sendable, Equatable {
    public let app: UInt64
    public let scene: UInt64
    public let owner: UInt64
    public let generation: UInt64
    public let expectedRevision: UInt64
    public let encodedBytes: UInt32
    public let operations: UInt32
    public init(app: UInt64, scene: UInt64, owner: UInt64,
                generation: UInt64, expectedRevision: UInt64,
                encodedBytes: UInt32, operations: UInt32) {
        self.app = app
        self.scene = scene
        self.owner = owner
        self.generation = generation
        self.expectedRevision = expectedRevision
        self.encodedBytes = encodedBytes
        self.operations = operations
    }
}
