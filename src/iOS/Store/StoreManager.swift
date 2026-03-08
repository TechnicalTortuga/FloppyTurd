//
//  StoreManager.swift
//  PooperTrooper
//
//  Created by AI Assistant
//  StoreKit 2 implementation for In-App Purchases
//  Handles "Remove Ads" IAP with server-validated receipt checking
//

import Foundation
import GameCorePlatform
import StoreKit

/// @brief Singleton manager for StoreKit 2 In-App Purchases
/// Handles purchase, restoration, and validation of "Remove Ads" product
@MainActor
class StoreManager: NSObject {
    
    // MARK: - Singleton
    
    static let shared = StoreManager()
    
    // MARK: - Properties
    
    /// Product ID for "Remove Ads" IAP
    static let removeAdsProductID = "com.floppyturd.game.removeads"
    
    /// Cached product information
    private(set) var removeAdsProduct: Product?
    
    /// Current purchase state
    private(set) var hasPurchasedRemoveAds: Bool = false
    
    /// Transaction listener task
    private var transactionListener: Task<Void, Error>?
    
    // MARK: - Initialization
    
    private override init() {
        super.init()
        SwiftLog.info("💰 [StoreManager] Initializing...", category: "StoreManager")
        
        // Start listening for transactions
        transactionListener = listenForTransactions()
        
        // Load products and check purchase status
        Task {
            await loadProducts()
            await checkPurchaseStatus()
        }
    }
    
    deinit {
        transactionListener?.cancel()
    }
    
    // MARK: - Product Loading
    
    /// Load available products from App Store
    func loadProducts() async {
        SwiftLog.info("🔄 [StoreManager] Loading products from App Store...", category: "StoreManager")
        SwiftLog.info("🔄 [StoreManager] Product ID: \(Self.removeAdsProductID)", category: "StoreManager")
        
        do {
            let products = try await Product.products(for: [Self.removeAdsProductID])
            SwiftLog.info("✅ [StoreManager] Product.products() returned \(products.count) products", category: "StoreManager")
            
            if let product = products.first {
                removeAdsProduct = product
                SwiftLog.info("✅ [StoreManager] Loaded product: \(product.displayName) - \(product.displayPrice)", category: "StoreManager")
                SwiftLog.info("✅ [StoreManager] Product ID: \(product.id)", category: "StoreManager")
                SwiftLog.info("✅ [StoreManager] Product Type: \(product.type)", category: "StoreManager")
            } else {
                SwiftLog.warn("⚠️ [StoreManager] No products found for ID: \(Self.removeAdsProductID)", category: "StoreManager")
                SwiftLog.warn("⚠️ [StoreManager] This usually means:", category: "StoreManager")
                SwiftLog.warn("   1. Product not configured in App Store Connect", category: "StoreManager")
                SwiftLog.warn("   2. Product ID mismatch", category: "StoreManager")
                SwiftLog.warn("   3. App not signed with correct provisioning profile", category: "StoreManager")
            }
        } catch {
            SwiftLog.error("❌ [StoreManager] Failed to load products: \(error.localizedDescription)", category: "StoreManager")
            SwiftLog.error("❌ [StoreManager] Error type: \(type(of: error))", category: "StoreManager")
        }
    }
    
    /// Get the localized price string for display
    func getPriceString() -> String? {
        return removeAdsProduct?.displayPrice
    }
    
    // MARK: - Purchase Status Checking
    
    /// Check if user has already purchased "Remove Ads"
    /// This is called on app launch to restore purchase state
    func checkPurchaseStatus() async {
        SwiftLog.info("🔍 [StoreManager] Checking purchase status...", category: "StoreManager")
        
        // Check for valid transactions in App Store receipt
        for await result in Transaction.currentEntitlements {
            do {
                let transaction = try checkVerified(result)
                
                // Check if this is our "Remove Ads" product
                if transaction.productID == Self.removeAdsProductID {
                    SwiftLog.info("✅ [StoreManager] Found valid 'Remove Ads' purchase", category: "StoreManager")
                    hasPurchasedRemoveAds = true
                    
                    // Notify AdManager to disable ads
                    AdManager.shared.setAdsEnabled(false)
                    
                    // Finish the transaction (if not already finished)
                    await transaction.finish()
                    return
                }
            } catch {
                SwiftLog.warn("⚠️ [StoreManager] Transaction verification failed: \(error)", category: "StoreManager")
            }
        }
        
        SwiftLog.info("📊 [StoreManager] No valid 'Remove Ads' purchase found", category: "StoreManager")
        hasPurchasedRemoveAds = false
        
        // Ensure ads are enabled if no purchase found
        AdManager.shared.setAdsEnabled(true)
    }
    
    // MARK: - Purchase Flow
    
    /// Purchase the "Remove Ads" product
    /// - Parameter completion: Callback with success/failure and optional error
    func purchaseRemoveAds(completion: @escaping (Bool, Error?) -> Void) {
        SwiftLog.info("🔵 [StoreManager] purchaseRemoveAds() called", category: "StoreManager")
        
        // If product not loaded yet, try loading it first
        guard let product = removeAdsProduct else {
            SwiftLog.warn("⚠️ [StoreManager] Product not loaded yet - attempting to load now...", category: "StoreManager")
            Task {
                await loadProducts()
                
                // Check again after loading
                guard let product = removeAdsProduct else {
                    SwiftLog.error("❌ [StoreManager] Cannot purchase - product failed to load", category: "StoreManager")
                    SwiftLog.error("❌ [StoreManager] Possible reasons:", category: "StoreManager")
                    SwiftLog.error("   1. Product ID '\(Self.removeAdsProductID)' not configured in App Store Connect", category: "StoreManager")
                    SwiftLog.error("   2. App not signed with correct provisioning profile", category: "StoreManager")
                    SwiftLog.error("   3. Network connection issue", category: "StoreManager")
                    completion(false, StoreError.productNotLoaded)
                    return
                }
                
                // Product loaded successfully, proceed with purchase
                await self.performPurchase(product: product, completion: completion)
            }
            return
        }
        
        // Product already loaded, proceed with purchase
        Task {
            await performPurchase(product: product, completion: completion)
        }
    }
    
    /// Internal helper to perform the actual purchase
    private func performPurchase(product: Product, completion: @escaping (Bool, Error?) -> Void) async {
        SwiftLog.info("💳 [StoreManager] Initiating purchase for: \(product.displayName)", category: "StoreManager")
        SwiftLog.info("💳 [StoreManager] Product ID: \(product.id)", category: "StoreManager")
        SwiftLog.info("💳 [StoreManager] Product Price: \(product.displayPrice)", category: "StoreManager")
        
        do {
            SwiftLog.info("🔄 [StoreManager] Calling product.purchase()...", category: "StoreManager")
            let result = try await product.purchase()
            SwiftLog.info("✅ [StoreManager] product.purchase() returned result: \(result)", category: "StoreManager")
            
            switch result {
            case .success(let verification):
                // Transaction successful - verify it
                let transaction = try checkVerified(verification)
                
                SwiftLog.info("✅ [StoreManager] Purchase successful - ID: \(transaction.id)", category: "StoreManager")
                
                // Update state
                hasPurchasedRemoveAds = true
                
                // Disable ads immediately
                AdManager.shared.setAdsEnabled(false)
                
                // Finish the transaction
                await transaction.finish()
                
                // Notify success
                completion(true, nil)
                
            case .userCancelled:
                SwiftLog.warn("⚠️ [StoreManager] User cancelled purchase", category: "StoreManager")
                completion(false, StoreError.userCancelled)
                
            case .pending:
                SwiftLog.warn("⏳ [StoreManager] Purchase pending (awaiting approval)", category: "StoreManager")
                SwiftLog.warn("⏳ [StoreManager] This usually means parental approval is required", category: "StoreManager")
                completion(false, StoreError.purchasePending)
                
            @unknown default:
                SwiftLog.error("❌ [StoreManager] Unknown purchase result", category: "StoreManager")
                completion(false, StoreError.unknown)
            }
            
        } catch {
            SwiftLog.error("❌ [StoreManager] Purchase failed with exception: \(error.localizedDescription)", category: "StoreManager")
            SwiftLog.error("❌ [StoreManager] Error type: \(type(of: error))", category: "StoreManager")
            SwiftLog.error("❌ [StoreManager] Full error: \(error)", category: "StoreManager")
            completion(false, error)
        }
    }
    
    /// Restore previous purchases
    /// Call this when user taps "Restore Purchases" button
    func restorePurchases(completion: @escaping (Bool, Error?) -> Void) {
        SwiftLog.info("🔄 [StoreManager] Restoring purchases...", category: "StoreManager")
        
        Task {
            do {
                // Request App Store to sync all transactions
                try await AppStore.sync()
                
                // Re-check purchase status
                await checkPurchaseStatus()
                
                if hasPurchasedRemoveAds {
                    SwiftLog.info("✅ [StoreManager] Purchase restored successfully", category: "StoreManager")
                    completion(true, nil)
                } else {
                    SwiftLog.warn("⚠️ [StoreManager] No purchases to restore", category: "StoreManager")
                    completion(false, StoreError.noPurchasesToRestore)
                }
                
            } catch {
                SwiftLog.error("❌ [StoreManager] Restore failed: \(error.localizedDescription)", category: "StoreManager")
                completion(false, error)
            }
        }
    }
    
    // MARK: - Transaction Listening
    
    /// Listen for transaction updates from App Store
    /// This catches purchases made on other devices or from App Store directly
    private func listenForTransactions() -> Task<Void, Error> {
        return Task.detached {
            for await result in Transaction.updates {
                do {
                    let transaction = try await self.checkVerified(result)
                    
                    // Check if this is our "Remove Ads" product
                    let productID = await Self.removeAdsProductID
                    if transaction.productID == productID {
                        await MainActor.run {
                            print("🔔 [StoreManager] Transaction update received - removing ads")
                            self.hasPurchasedRemoveAds = true
                            AdManager.shared.setAdsEnabled(false)
                        }
                    }
                    
                    // Finish the transaction
                    await transaction.finish()
                    
                } catch {
                    print("⚠️ [StoreManager] Transaction update verification failed: \(error)")
                }
            }
        }
    }
    
    // MARK: - Verification
    
    /// Verify transaction using Apple's cryptographic signature
    /// This prevents receipt forgery and ensures purchases are legitimate
    private func checkVerified<T>(_ result: VerificationResult<T>) throws -> T {
        switch result {
        case .unverified(_, let error):
            // Transaction failed verification - could be tampered
            throw StoreError.verificationFailed(error)
        case .verified(let safe):
            // Transaction is cryptographically verified by Apple
            return safe
        }
    }
}

// MARK: - Store Errors

enum StoreError: LocalizedError {
    case productNotLoaded
    case userCancelled
    case purchasePending
    case noPurchasesToRestore
    case verificationFailed(Error)
    case unknown
    
    var errorDescription: String? {
        switch self {
        case .productNotLoaded:
            return "Product information not loaded. Please try again."
        case .userCancelled:
            return "Purchase was cancelled."
        case .purchasePending:
            return "Purchase is pending approval."
        case .noPurchasesToRestore:
            return "No previous purchases found to restore."
        case .verificationFailed(let error):
            return "Purchase verification failed: \(error.localizedDescription)"
        case .unknown:
            return "An unknown error occurred."
        }
    }
}
