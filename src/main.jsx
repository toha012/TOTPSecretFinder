import { StrictMode } from 'react'
import { createRoot } from 'react-dom/client'
import { BrowserRouter, Routes, Route } from 'react-router-dom'
import App from './App.jsx'
import PrecomputedPage from './PrecomputedPage.jsx'
import './index.css'

const basename = import.meta.env.BASE_URL

createRoot(document.getElementById('root')).render(
  <StrictMode>
    <BrowserRouter basename={basename}>
      <Routes>
        <Route path="/" element={<App />} />
        <Route path="/precomputed" element={<PrecomputedPage />} />
      </Routes>
    </BrowserRouter>
  </StrictMode>,
)
