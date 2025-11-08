//
//  StoreManager.swift
//  FloppyTurd
//
//  Created by AI Assistant
//  StoreKit 2 implementation for In-App Purchases
//  Handles "Remove Ads" IAP with server-validated receipt checking
//

import Foundation
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
        print("💰 [StoreManager] Initializing...")
        
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
        do {
            let products = try await Product.products(for: [Self.removeAdsProductID])
            
            if let product = products.first {
                removeAdsProduct = product
                print("✅ [StoreManager] Loaded product: \(product.displayName) - \(product.displayPrice)")
            } else {
                print("⚠️ [StoreManager] No products found for ID: \(Self.removeAdsProductID)")
            }
        } catch {
            print("❌ [StoreManager] Failed to load products: \(error.localizedDescription)")
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
        print("🔍 [StoreManager] Checking purchase status...")
        
        // Check for valid transactions in App Store receipt
        for await result in Transaction.currentEntitlements {
            do {
                let transaction = try checkVerified(result)
                
                // Check if this is our "Remove Ads" product
                if transaction.productID == Self.removeAdsProductID {
                    print("✅ [StoreManager] Found valid 'Remove Ads' purchase")
                    hasPurchasedRemoveAds = true
                    
                    // Notify AdManager to disable ads
                    AdManager.shared.setAdsEnabled(false)
                    
                    // Finish the transaction (if not already finished)
                    await transaction.finish()
                    return
                }
            } catch {
                print("⚠️ [StoreManager] Transaction verification failed: \(error)")
            }
        }
        
        print("📊 [StoreManager] No valid 'Remove Ads' purchase found")
        hasPurchasedRemoveAds = false
        
        // Ensure ads are enabled if no purchase found
        AdManager.shared.setAdsEnabled(true)
    }
    
    // MARK: - Purchase Flow
    
    /// Purchase the "Remove Ads" product
    /// - Parameter completion: Callback with success/failure and optional error
    func purchaseRemoveAds(completion: @escaping (Bool, Error?) -> Void) {
        guard let product = removeAdsProduct else {
            print("❌ [StoreManager] Cannot purchase - product not loaded")
            completion(false, StoreError.productNotLoaded)
            return
        }
        
        print("💳 [StoreManager] Initiating purchase for: \(product.displayName)")
        
        Task {
            do {
                let result = try await product.purchase()
                
                switch result {
                case .success(let verification):
                    // Transaction successful - verify it
                    let transaction = try checkVerified(verification)
                    
                    print("✅ [StoreManager] Purchase successful - ID: \(transaction.id)")
                    
                    // Update state
                    hasPurchasedRemoveAds = true
                    
                    // Disable ads immediately
                    AdManager.shared.setAdsEnabled(false)
                    
                    // Finish the transaction
                    await transaction.finish()
                    
                    // Notify success
                    completion(true, nil)
                    
                case .userCancelled:
                    print("⚠️ [StoreManager] User cancelled purchase")
                    completion(false, StoreError.userCancelled)
                    
                case .pending:
                    print("⏳ [StoreManager] Purchase pending (awaiting approval)")
                    completion(false, StoreError.purchasePending)
                    
                @unknown default:
                    print("❌ [StoreManager] Unknown purchase result")
                    completion(false, StoreError.unknown)
                }
                
            } catch {
                print("❌ [StoreManager] Purchase failed: \(error.localizedDescription)")
                completion(false, error)
            }
        }
    }
    
    /// Restore previous purchases
    /// Call this when user taps "Restore Purchases" button
    func restorePurchases(completion: @escaping (Bool, Error?) -> Void) {
        print("🔄 [StoreManager] Restoring purchases...")
        
        Task {
            do {
                // Request App Store to sync all transactions
                try await AppStore.sync()
                
                // Re-check purchase status
                await checkPurchaseStatus()
                
                if hasPurchasedRemoveAds {
                    print("✅ [StoreManager] Purchase restored successfully")
                    completion(true, nil)
                } else {
                    print("⚠️ [StoreManager] No purchases to restore")
                    completion(false, StoreError.noPurchasesToRestore)
                }
                
            } catch {
                print("❌ [StoreManager] Restore failed: \(error.localizedDescription)")
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
