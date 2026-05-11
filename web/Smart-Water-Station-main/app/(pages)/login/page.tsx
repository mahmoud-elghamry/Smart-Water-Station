export default function LoginPage() {
  return (
    <main className="w-full min-h-screen bg-gray-50 p-6 flex items-center justify-center">
      <div className="w-full max-w-md">
        <div className="bg-white rounded-lg shadow p-8">
          <h1 className="text-4xl font-bold mb-2" style={{ color: '#4169b3' }}>Login</h1>
          <p className="text-gray-600 mb-8">Welcome to AquaPuer - Smart Water Management System</p>
          
          <form className="space-y-6">
            <div>
              <label htmlFor="email" className="block text-sm font-semibold mb-2" style={{ color: '#4169b3' }}>Email Address</label>
              <input
                id="email"
                type="email"
                placeholder="Enter your email"
                className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2" 
                
              />
            </div>
            
            <div>
              <label htmlFor="password" className="block text-sm font-semibold mb-2" style={{ color: '#4169b3' }}>Password</label>
              <input
                id="password"
                type="password"
                placeholder="Enter your password"
                className="w-full px-4 py-2 border border-gray-300 rounded-lg focus:outline-none focus:ring-2"
                
              />
            </div>
            
            <button
              type="submit"
              className="w-full py-2 rounded-lg font-semibold text-white transition hover:opacity-90"
              style={{ backgroundColor: '#4169b3' }}
            >
              Sign In
            </button>
          </form>
          
          <p className="text-center text-sm text-gray-600 mt-6">
            Don't have an account?{' '}
            <a href="#" className="font-semibold hover:underline" style={{ color: '#4169b3' }}>
              Sign up here
            </a>
          </p>
        </div>
      </div>
    </main>
  );
}
