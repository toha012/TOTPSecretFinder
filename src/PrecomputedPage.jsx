import { useState, useEffect, useMemo } from 'react'
import { Link } from 'react-router-dom'
import { QRCodeSVG } from 'qrcode.react'
import DatePicker from 'react-datepicker'
import 'react-datepicker/dist/react-datepicker.css'
import './App.css'
import './PrecomputedPage.css'

// Base32 encoding
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
    while (result.length % 8 !== 0) {
        result += '=';
    }
    return result;
}

function stepToDate(step) {
    return new Date(step * 30 * 1000);
}

function hexToBytes(hex) {
    const bytes = [];
    for (let i = 0; i < hex.length; i += 2) {
        bytes.push(parseInt(hex.substr(i, 2), 16));
    }
    return bytes;
}

function PrecomputedPage() {
    const [availableDates, setAvailableDates] = useState([])
    const [selectedDate, setSelectedDate] = useState(null)
    const [results, setResults] = useState([])
    const [loading, setLoading] = useState(false)
    const [selectedResult, setSelectedResult] = useState(null)
    const [issuer, setIssuer] = useState('TOTP-Finder')
    const [accountName, setAccountName] = useState('')

    useEffect(() => {
        fetch('./precomputed_data/index.json')
            .then(res => res.json())
            .then(data => setAvailableDates(data.dates || []))
            .catch(() => setAvailableDates([]))
    }, [])

    const handleDateSelect = async (dateStr) => {
        setSelectedDate(dateStr)
        setLoading(true)
        setResults([])

        try {
            const res = await fetch(`./precomputed_data/${dateStr}.txt`)
            const text = await res.text()
            const lines = text.trim().split('\n').filter(l => l.length > 0)

            const [year, month, day] = dateStr.split('-').map(Number)
            const dayStart = new Date(year, month - 1, day, 0, 0, 0)
            const stepStart = Math.floor(dayStart.getTime() / 30000)

            const parsed = lines.map((hex, index) => ({
                step: stepStart + index,
                secretHex: hex.trim(),
                secret: hexToBytes(hex.trim())
            }))

            setResults(parsed)
        } catch (e) {
            console.error('Failed to load precomputed data:', e)
        }
        setLoading(false)
    }

    useEffect(() => {
        if (selectedResult) {
            const date = stepToDate(selectedResult.step)
            const formatted = `${date.getFullYear()}${(date.getMonth()+1).toString().padStart(2,'0')}${date.getDate().toString().padStart(2,'0')}_${date.getHours().toString().padStart(2,'0')}${date.getMinutes().toString().padStart(2,'0')}${date.getSeconds().toString().padStart(2,'0')} (000000)`
            setAccountName(formatted)
        }
    }, [selectedResult])

    const getOtpAuthUrl = (result) => {
        const secretBase32 = base32Encode(result.secret)
        return `otpauth://totp/${encodeURIComponent(issuer)}:${encodeURIComponent(accountName)}?secret=${secretBase32}&issuer=${encodeURIComponent(issuer)}&algorithm=SHA1&digits=6&period=30`
    }

    const isDateAvailable = (dateStr) => availableDates.includes(dateStr)

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
                            <Link to="/" className="nav-link">Search</Link>
                            <Link to="/precomputed" className="nav-link active">Precomputed</Link>
                        </nav>
                    </div>
                    <p className="subtitle">Precomputed secrets (Target code: 000000)</p>
                </div>
            </header>

            <main className="main">
                <div className="container precomputed-container">
                    <section className="config-section">
                        <h2 className="section-title">日付を選択</h2>
                        {availableDates.length > 0 ? (
                            <div className="form-group">
                                <label className="label">データのある日付を選択してください</label>
                                <DatePicker
                                    selected={selectedDate ? new Date(selectedDate + 'T00:00:00') : null}
                                    onChange={(date) => {
                                        if (date) {
                                            const year = date.getFullYear()
                                            const month = String(date.getMonth() + 1).padStart(2, '0')
                                            const day = String(date.getDate()).padStart(2, '0')
                                            const dateStr = `${year}-${month}-${day}`
                                            handleDateSelect(dateStr)
                                        }
                                    }}
                                    filterDate={(date) => {
                                        const year = date.getFullYear()
                                        const month = String(date.getMonth() + 1).padStart(2, '0')
                                        const day = String(date.getDate()).padStart(2, '0')
                                        const dateStr = `${year}-${month}-${day}`
                                        return availableDates.includes(dateStr)
                                    }}
                                    dateFormat="yyyy-MM-dd"
                                    placeholderText="日付を選択してください"
                                    className="input date-picker-input"
                                    calendarClassName="date-picker-calendar"
                                />
                            </div>
                        ) : (
                            <p className="no-data-hint">利用可能なデータがありません</p>
                        )}
                    </section>

                    {loading && <div className="loading">読み込み中...</div>}

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
                                <p className="no-results-hint">日付を選択すると、その日のシークレットが表示されます</p>
                            </div>
                        ) : (
                            <div className="results-list">
                                {results.map((result, index) => (
                                    <div
                                        key={index}
                                        className={`result-item ${selectedResult === result ? 'selected' : ''}`}
                                        onClick={() => setSelectedResult(result)}
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
                </div>
            </main>

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
                                    表示された時刻にターゲットコード「000000」が表示されます。
                                </p>
                            </div>
                        </div>
                    </div>
                </div>
            )}
        </div>
    )
}

export default PrecomputedPage