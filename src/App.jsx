import { useState, useRef, useCallback, useEffect } from 'react'
import { Link, useLocation } from 'react-router-dom'
import { QRCodeSVG } from 'qrcode.react'
import './App.css'

// Base32 encoding for Google Authenticator
function base32Encode(bytes) {
  const alphabet = 'ABCDEFGHIJKLMNOPQRSTUVWXYZ234567';
  let result = '';
  let bits = 0;
  let value = 0;
  
  for (let i = 0; i < bytes.length; i++) {
    value = (value << 8) | bytes[i];
    bits += 8;
    
    while (bits >= 5) {
      result += alphabet[(value >>> (bits - 5)) & 31];
      bits -= 5;
    }
  }
  
  if (bits > 0) {
    result += alphabet[(value << (5 - bits)) & 31];
  }
  
  // Add padding
  while (result.length % 8 !== 0) {
    result += '=';
  }
  
  return result;
}

// Time step to date conversion
function stepToDate(step) {
  const timestamp = step * 30 * 1000;
  return new Date(timestamp);
}

// Calculate failure probability
function calculateFailureProbabilityByStep(randomSize, secretLength) {
  const p_not_find_single = (1000000 - 1) / 1000000;
  const attempts_per_step = randomSize - secretLength;
  const p_not_find_step = Math.pow(p_not_find_single, attempts_per_step);
  return p_not_find_step;
}

function formatDateTimeWithSeconds(date) {
  const pad = (n) => n.toString().padStart(2, '0');
  // 秒を0か30に丸める
  const seconds = date.getSeconds() < 30 ? 0 : 30;
  console.log(`${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}T${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(seconds)}`);1
  return `${date.getFullYear()}-${pad(date.getMonth() + 1)}-${pad(date.getDate())}T${pad(date.getHours())}:${pad(date.getMinutes())}:${pad(seconds)}`;
}

function parseDateTimeWithSeconds(str) {
  // "YYYY-MM-DDTHH:MM:SS" 形式をパース
  console.log(str);
  const [datePart, timePart] = str.split('T');
  const [year, month, day] = datePart.split('-').map(Number);
  const [hour, minute, second] = timePart.split(':').map(Number);
  return new Date(year, month - 1, day, hour, minute, second || 0);
}

function App() {
  const now = new Date();
  const fiveMinutesLater = new Date(now.getTime() + (5 * 60 - 30) * 1000);

  const [target, setTarget] = useState('000000')
  const [startTime, setStartTime] = useState(formatDateTimeWithSeconds(now))
  const [endTime, setEndTime] = useState(formatDateTimeWithSeconds(fiveMinutesLater))
  const [randomSize, setRandomSize] = useState(16)
  const [isSearching, setIsSearching] = useState(false)
  const [progress, setProgress] = useState({ step: 0, total: 0, percent: 0 })
  const [results, setResults] = useState([])
  const [selectedResult, setSelectedResult] = useState(null)
  const [issuer, setIssuer] = useState('TOTP-Finder')
  const [accountName, setAccountName] = useState('')
  const [error, setError] = useState('')
  const [workerReady, setWorkerReady] = useState(false)
  
  const workerRef = useRef(null)
  
  useEffect(() => {
    workerRef.current = new Worker(
      new URL('./workers/searchWorker.js', import.meta.url),
      { type: 'module' }
    )
    
    workerRef.current.onmessage = (e) => {
      const { type, ...data } = e.data
      
      switch (type) {
        case 'initialized':
          console.log('Worker initialized with', data.size, 'bytes')
          setWorkerReady(true)
          break
        case 'progress':
          setProgress({
            step: data.currentStep,
            total: data.totalSteps,
            percent: Math.round((data.currentStep / data.totalSteps) * 100 + 
                               (data.index / data.maxIndex / data.totalSteps) * 100)
          })
          break
        case 'stepComplete':
          setProgress({
            step: data.currentStep,
            total: data.totalSteps,
            percent: Math.round((data.currentStep / data.totalSteps) * 100)
          })
          break
        case 'found':
          setResults(prev => [...prev, data.result])
          break
        case 'complete':
          setIsSearching(false)
          break
      }
    }
    
    return () => {
      workerRef.current?.terminate()
    }
  }, [])
  
  useEffect(() => {
    if (workerRef.current) {
      setWorkerReady(false)
      workerRef.current.postMessage({
        type: 'init',
        data: { randomSize: randomSize * 1024 * 1024 }
      })
    }
  }, [randomSize])

  useEffect(() => {
    if (selectedResult) {
      const date = stepToDate(selectedResult.step)
      const formatted = `${date.getFullYear()}${(date.getMonth()+1).toString().padStart(2,'0')}${date.getDate().toString().padStart(2,'0')}_${date.getHours().toString().padStart(2,'0')}${date.getMinutes().toString().padStart(2,'0')}${date.getSeconds().toString().padStart(2,'0')} (${target.padStart(6,'0')})`
      setAccountName(formatted)
    }
  }, [selectedResult, target])
  
  const handleStart = useCallback(() => {
    if (!target || target.length !== 6 || !/^\d{6}$/.test(target)) {
      setError('ターゲットは6桁の数字で入力してください')
      return
    }
    
    if (!startTime || !endTime) {
      setError('開始時刻と終了時刻を入力してください')
      return
    }
    
    const start = parseDateTimeWithSeconds(startTime)
    const end = parseDateTimeWithSeconds(endTime)
    
    if (start >= end) {
      setError('終了時刻は開始時刻より後にしてください')
      return
    }
    
    setError('')
    setResults([])
    setSelectedResult(null)
    setIsSearching(true)
    
    const stepStart = Math.floor(start.getTime() / 30000)
    const stepEnd = Math.floor(end.getTime() / 30000)
    
    workerRef.current?.postMessage({
      type: 'search',
      data: {
        stepStart,
        stepEnd,
        target: parseInt(target, 10)
      }
    })
  }, [target, startTime, endTime])
  
  const handleCancel = useCallback(() => {
    workerRef.current?.postMessage({ type: 'cancel' })
  }, [])
  
  const handleResultClick = useCallback((result) => {
    setSelectedResult(result)
  }, [])
  
  const failureProbability = calculateFailureProbabilityByStep(randomSize * 1024 * 1024, 20)
  const successProbability = (1 - failureProbability) * 100

  function formatSuccessProbability(prob) {
    if (prob > 99.999999) {
      return '> 99.999999'
    }
    return prob.toFixed(6)
  }
  
  const getOtpAuthUrl = (result) => {
    const secretBase32 = base32Encode(result.secret)
    return `otpauth://totp/${encodeURIComponent(issuer)}:${encodeURIComponent(accountName)}?secret=${secretBase32}&issuer=${encodeURIComponent(issuer)}&algorithm=SHA1&digits=6&period=30`
  }

  return (
    <div className="app">
      <header className="header">
        <div className="header-content">
          <div className="header-top">
            <h1 className="title">
              <span className="title-icon">🔐</span>
              TOTP Secret Finder
            </h1>
            <nav className="header-nav">
              <Link to="/" className="nav-link active">Search</Link>
              <Link to="/precomputed" className="nav-link">Precomputed</Link>
            </nav>
          </div>
          <p className="subtitle">Find TOTP secrets that generate specific codes</p>
        </div>
      </header>
      
      <main className="main">
        <div className="container">
          <section className="config-section">
            <h2 className="section-title">検索設定</h2>
            
            <div className="form-grid">
              <div className="form-group">
                <label className="label">
                  ターゲットコード
                  <span className="label-hint">6桁の数字</span>
                </label>
                <input
                  type="text"
                  className="input input-large"
                  value={target}
                  onChange={(e) => setTarget(e.target.value.replace(/\D/g, '').slice(0, 6))}
                  placeholder="000000"
                  maxLength={6}
                  disabled={isSearching}
                />
              </div>
              
              <div className="form-group">
                <label className="label">ランダムデータサイズ</label>
                <div className="select-wrapper">
                  <select
                    className="select"
                    value={randomSize}
                    onChange={(e) => setRandomSize(parseInt(e.target.value))}
                    disabled={isSearching}
                  >
                    <option value={1}>1 MB</option>
                    <option value={4}>4 MB</option>
                    <option value={8}>8 MB</option>
                    <option value={16}>16 MB (推奨)</option>
                    <option value={32}>32 MB</option>
                  </select>
                </div>
              </div>
              
              <div className="form-group">
                <label className="label">開始時刻</label>
                <div className="datetime-with-seconds">
                  <input
                    type="datetime-local"
                    className="input"
                    value={startTime.slice(0, 16)}
                    onChange={(e) => {
                      const currentSeconds = startTime.slice(16) || ':00'
                      setStartTime(e.target.value + currentSeconds)
                    }}
                    disabled={isSearching}
                  />
                  <select
                    className="seconds-select"
                    value={startTime.slice(16) || ':00'}
                    onChange={(e) => setStartTime(startTime.slice(0, 16) + e.target.value)}
                    disabled={isSearching}
                  >
                    <option value=":00">:00</option>
                    <option value=":30">:30</option>
                  </select>
                </div>
              </div>

              <div className="form-group">
                <label className="label">終了時刻</label>
                <div className="datetime-with-seconds">
                  <input
                    type="datetime-local"
                    className="input"
                    value={endTime.slice(0, 16)}
                    onChange={(e) => {
                      const currentSeconds = endTime.slice(16) || ':00'
                      setEndTime(e.target.value + currentSeconds)
                    }}
                    disabled={isSearching}
                  />
                  <select
                    className="seconds-select"
                    value={endTime.slice(16) || ':00'}
                    onChange={(e) => setEndTime(endTime.slice(0, 16) + e.target.value)}
                    disabled={isSearching}
                  >
                    <option value=":00">:00</option>
                    <option value=":30">:30</option>
                  </select>
                </div>
              </div>
            </div>
            
            <div className="probability-info">
              <div className="probability-label">各ステップで見つかる確率:</div>
              <div className="probability-value">
                {formatSuccessProbability(successProbability)}%
              </div>
              <div className="probability-detail">
                ({randomSize}MB)
              </div>
            </div>
            
            {error && (
              <div className="error-message">
                <span className="error-icon">⚠️</span>
                {error}
              </div>
            )}
            
            <div className="button-group">
              {!isSearching ? (
                <button 
                  className="btn btn-primary" 
                  onClick={handleStart}
                  disabled={!workerReady}
                >
                  <span className="btn-icon">🔍</span>
                  {workerReady ? '探索開始' : '初期化中...'}
                </button>
              ) : (
                <button className="btn btn-cancel" onClick={handleCancel}>
                  <span className="btn-icon">✕</span>
                  キャンセル
                </button>
              )}
            </div>
            
            {isSearching && (
              <div className="progress-container">
                <div className="progress-bar">
                  <div 
                    className="progress-fill"
                    style={{ width: `${progress.percent}%` }}
                  />
                </div>
                <div className="progress-text">
                  ステップ {progress.step} / {progress.total} ({progress.percent}%)
                </div>
              </div>
            )}
          </section>
          
          <section className="results-section">
            <h2 className="section-title">
              検索結果
              {results.length > 0 && (
                <span className="results-count">{results.length}件</span>
              )}
            </h2>
            
            {results.length === 0 ? (
              <div className="no-results">
                <div className="no-results-icon">📋</div>
                <p>まだ結果がありません</p>
                <p className="no-results-hint">探索を開始すると、見つかったシークレットがここに表示されます</p>
              </div>
            ) : (
              <div className="results-list">
                {results.map((result, index) => (
                  <div
                    key={index}
                    className={`result-item ${selectedResult === result ? 'selected' : ''}`}
                    onClick={() => handleResultClick(result)}
                  >
                    <div className="result-header">
                      <span className="result-index">#{index + 1}</span>
                      <span className="result-time">
                        {stepToDate(result.step).toLocaleString('ja-JP')}
                      </span>
                    </div>
                    <div className="result-secret">
                      <code>{result.secretHex}</code>
                    </div>
                    <div className="result-step">
                      Time Step: {result.step}
                    </div>
                  </div>
                ))}
              </div>
            )}
          </section>
          
          {selectedResult && (
            <div className="modal-overlay" onClick={() => setSelectedResult(null)}>
              <div className="modal-content" onClick={(e) => e.stopPropagation()}>
                <button className="modal-close" onClick={() => setSelectedResult(null)}>×</button>
                <h2 className="section-title">QRコード生成</h2>
                
                <div className="qr-config">
                  <div className="form-group">
                    <label className="label">発行者名</label>
                    <input
                      type="text"
                      className="input"
                      value={issuer}
                      onChange={(e) => setIssuer(e.target.value)}
                      placeholder="TOTP-Finder"
                    />
                  </div>
                  <div className="form-group">
                    <label className="label">アカウント名</label>
                    <input
                      type="text"
                      className="input"
                      value={accountName}
                      onChange={(e) => setAccountName(e.target.value)}
                      placeholder="user"
                    />
                  </div>
                </div>
                
                <div className="qr-display">
                  <div className="qr-code">
                    <QRCodeSVG
                      value={getOtpAuthUrl(selectedResult)}
                      size={200}
                      level="M"
                      includeMargin={true}
                      bgColor="#ffffff"
                      fgColor="#1e293b"
                    />
                  </div>
                  <div className="qr-info">
                    <div className="qr-info-item">
                      <span className="qr-info-label">シークレット (Base32):</span>
                      <code className="qr-info-value">{base32Encode(selectedResult.secret)}</code>
                    </div>
                    <div className="qr-info-item">
                      <span className="qr-info-label">時刻:</span>
                      <span className="qr-info-value">
                        {stepToDate(selectedResult.step).toLocaleString('ja-JP')}
                      </span>
                    </div>
                    <div className="qr-info-item">
                      <span className="qr-info-label">URL:</span>
                      <code className="qr-info-value qr-url">{getOtpAuthUrl(selectedResult)}</code>
                    </div>
                    <p className="qr-instructions">
                      Google Authenticatorでこのコードをスキャンしてください。
                      表示された時刻にターゲットコード「{target}」が表示されます。
                    </p>
                  </div>
                </div>
              </div>
            </div>
          )}
        </div>
      </main>
      
      <footer className="footer">
        <p>TOTP Secret Finder - Web Worker Powered Search</p>
      </footer>
    </div>
  )
}

export default App
